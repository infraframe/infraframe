#include "VideoDecoder.h"
#include "GstMappedFrame.h"
#include "OutPtr.h"

#include <libyuv.h>

using namespace infraframe;
using namespace std;

DEFINE_LOGGER(GStreamerVideoDecoder, "infraframe.GStreamerVideoDecoder");

constexpr size_t GstDecoderBufferSize = 20 * 1024 * 1024;
constexpr size_t WebrtcBufferPoolSize = 300;

GStreamerVideoDecoder::GStreamerVideoDecoder(string mediaTypeCaps,
    string decoderPipeline,
    bool resetPipelineOnSizeChanges)
    : _mediaTypeCaps { std::move(mediaTypeCaps) }
    , _decoderPipeline(std::move(decoderPipeline))
    , _resetPipelineOnSizeChanges(resetPipelineOnSizeChanges)
    , _keyframeNeeded { true }
    , _firstBufferPts { GST_CLOCK_TIME_NONE }
    , _firstBufferDts { GST_CLOCK_TIME_NONE }
    , _width { 0 }
    , _height { 0 }
    , _imageReadyCb { nullptr }
{
}

int32_t GStreamerVideoDecoder::Release()
{
    if (_gstDecoderPipeline) {
        _gstDecoderPipeline.reset();
    }

    _bufferManager.Release();

    return WEBRTC_VIDEO_CODEC_OK;
}
// int32_t GStreamerVideoDecoder::Decode(const webrtc::EncodedImage& inputImage,
//     bool missingFrames,
//     int64_t renderTimeMs)
int32_t GStreamerVideoDecoder::Decode(const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    int64_t renderTimeMs)
{
    if (_keyframeNeeded) {
        if (inputImage._frameType != webrtc::FrameType::kVideoFrameKey) {
            GST_WARNING(
                "Waiting for keyframe but got a delta unit... asking for keyframe");
            return WEBRTC_VIDEO_CODEC_ERROR;
        } else {
            initializeBufferTimestamps(renderTimeMs, inputImage._timeStamp);
            _keyframeNeeded = false;
        }
    }

    if (!_gstDecoderPipeline) {
        ELOG_ERROR("No source set, can't decode.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (_resetPipelineOnSizeChanges && inputImage._frameType == webrtc::FrameType::kVideoFrameKey && _width != 0 && _height != 0 && (_width != inputImage._encodedWidth || _height != inputImage._encodedHeight)) {
        initializePipeline();
        initializeBufferTimestamps(renderTimeMs, inputImage._timeStamp);
    }

    auto sample = toGstSample(inputImage, renderTimeMs);
    if (!sample) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    switch (_gstDecoderPipeline->pushSample(sample)) {
    case GST_FLOW_OK:
        ELOG_DEBUG("_gstDecoderPipeline->pushSample OK");
        break;
    case GST_FLOW_FLUSHING:
        ELOG_DEBUG("_gstDecoderPipeline->pushSample UNINITIALIZED");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    default:
        ELOG_DEBUG("_gstDecoderPipeline->pushSample ERROR");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return pullSample(renderTimeMs, inputImage._timeStamp, inputImage.rotation_);
}

// bool GStreamerVideoDecoder::Configure(
//     const webrtc::VideoDecoder::Settings& settings)
int32_t GStreamerVideoDecoder::InitDecode(const webrtc::VideoCodec* codec_settings,
    int32_t number_of_cores)
{
    _keyframeNeeded = true;

    if (!initializePipeline()) {
        ELOG_DEBUG("initializePipeline FAILED");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    if (!_gstreamerBufferPool.initialize(GstDecoderBufferSize)) {
        ELOG_DEBUG("_gstreamerBufferPool.initialize FAILED");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback)
{
    _imageReadyCb = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

bool GStreamerVideoDecoder::initializePipeline()
{
    _gstDecoderPipeline = make_unique<GStreamerDecoderPipeline>();
    return _gstDecoderPipeline->initialize(_mediaTypeCaps, _decoderPipeline) == WEBRTC_VIDEO_CODEC_OK;
}

void GStreamerVideoDecoder::initializeBufferTimestamps(
    int64_t renderTimeMs,
    uint32_t imageTimestamp)
{
    _firstBufferPts = (static_cast<guint64>(renderTimeMs)) * GST_MSECOND;
    _firstBufferDts = (static_cast<guint64>(imageTimestamp)) * GST_MSECOND;
}

gst::unique_ptr<GstSample>
GStreamerVideoDecoder::toGstSample(const webrtc::EncodedImage& inputImage,
    int64_t renderTimeMs)
{
    gst::unique_ptr<GstBuffer> buffer = _gstreamerBufferPool.acquireBuffer();
    if (!buffer) {
        return nullptr;
    }
    if (inputImage._length > gst_buffer_get_size(buffer.get())) {
        ELOG_ERROR("The buffer is too small");
        return nullptr;
    }
    // NOTE! EncodedImage::_size is the size of the buffer (think capacity of
    //       an std::vector) and EncodedImage::_length is the actual size of
    //       the bitstream (think size of an std::vector).
    gst_buffer_fill(buffer.get(), 0, inputImage._buffer, inputImage._length);
    gst_buffer_set_size(buffer.get(), static_cast<gssize>(inputImage._length));

    GST_BUFFER_DTS(buffer.get()) = (static_cast<guint64>(inputImage._timeStamp) * GST_MSECOND) - _firstBufferDts;
    GST_BUFFER_PTS(buffer.get()) = (static_cast<guint64>(renderTimeMs) * GST_MSECOND) - _firstBufferPts;

    ELOG_DEBUG("inputImage: [%d]-[%d]-[%d]", inputImage._length, inputImage._encodedWidth, inputImage._encodedHeight);
    return gst::unique_from_ptr(gst_sample_new(buffer.get(), getCapsForFrame(inputImage), nullptr, nullptr));
}

int32_t GStreamerVideoDecoder::pullSample(int64_t renderTimeMs,
    uint32_t imageTimestamp,
    webrtc::VideoRotation rotation)
{
    ELOG_DEBUG("GStreamerVideoDecoder::pullSample(%d,%lu)", renderTimeMs, imageTimestamp);
    GstState state;
    GstState pending;
    _gstDecoderPipeline->getSinkState(state, pending);
    ELOG_DEBUG("state [%s], pending [%s]",
        gst_element_state_get_name(state),
        gst_element_state_get_name(pending));

    auto sample = _gstDecoderPipeline->tryPullSample();
    if (!sample) {
        if (!_gstDecoderPipeline->ready() || state != GST_STATE_PLAYING) {
            ELOG_ERROR("NOT READY");
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        } else {
            ELOG_ERROR("NEEDS MORE DATA");
            return WEBRTC_VIDEO_CODEC_OK;
        }
    } else {
        ELOG_DEBUG("PULL SAMPLE OK!");
    }

    ELOG_DEBUG("");
    ELOG_DEBUG("setReady(true)");
    _gstDecoderPipeline->setReady(true);

    ELOG_DEBUG("mappedFrame");
    GstMappedFrame mappedFrame(sample.get(), GST_MAP_READ);
    if (!mappedFrame) {
        ELOG_ERROR("COULD NOT MAP FRAME");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (mappedFrame.format() != GST_VIDEO_FORMAT_I420) {
        ELOG_ERROR("WRONG FORMAT: IT MUST BE I420");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ELOG_DEBUG("i420Buffer");
    // auto i420Buffer = m_bufferManager->getFreeBuffer(mappedFrame.width(), mappedFrame.height());
    auto i420Buffer = _bufferManager.CreateBuffer(mappedFrame.width(), mappedFrame.height());
    if (!i420Buffer) {
        ELOG_ERROR("COULD NOT CREATE AN I420 BUFFER");
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }

    ELOG_DEBUG("libyuv::I420Copy");
    libyuv::I420Copy(mappedFrame.componentData(0),
        mappedFrame.componentStride(0),
        mappedFrame.componentData(1),
        mappedFrame.componentStride(1),
        mappedFrame.componentData(2),
        mappedFrame.componentStride(2),
        i420Buffer->MutableDataY(),
        i420Buffer->StrideY(),
        i420Buffer->MutableDataU(),
        i420Buffer->StrideU(),
        i420Buffer->MutableDataV(),
        i420Buffer->StrideV(),
        mappedFrame.width(),
        mappedFrame.height());

    if (_imageReadyCb) {
        ELOG_DEBUG("webrtc::VideoFrame");
        // webrtc::VideoFrame decodedImage = webrtc::VideoFrame::Builder()
        //                                       .set_video_frame_buffer(i420Buffer)
        //                                       .set_timestamp_rtp(imageTimestamp)
        //                                       .set_timestamp_ms(renderTimeMs)
        //                                       .set_rotation(rotation)
        //                                       .build();
        webrtc::VideoFrame decodedImage = webrtc::VideoFrame(i420Buffer, imageTimestamp, renderTimeMs, rotation);
        // _imageReadyCb->Decoded(decodedImage, absl::nullopt, absl::nullopt);
        ELOG_DEBUG("_imageReadyCb->Decoded");
        _imageReadyCb->Decoded(decodedImage);
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

GstCaps*
GStreamerVideoDecoder::getCapsForFrame(const webrtc::EncodedImage& image)
{
    ELOG_DEBUG("GStreamerVideoDecoder::getCapsForFrame [%s]", _mediaTypeCaps.c_str());
    gint lastWidth = _width;
    gint lastHeight = _height;
    _width = image._encodedWidth != 0 ? image._encodedWidth : _width;
    _height = image._encodedHeight != 0 ? image._encodedHeight : _height;

    ELOG_DEBUG("_width[%d], _height[%d]", _width, _height);
    ELOG_DEBUG("image._encodedWidth[%d], image._encodedHeight[%d]", image._encodedWidth, image._encodedHeight);

    if (!_caps || lastWidth != _width || lastHeight != _height) {
        if (_mediaTypeCaps == "video/x-h264") {
            _caps = gst::unique_from_ptr(gst_caps_new_simple(_mediaTypeCaps.c_str(),
                "width",
                G_TYPE_INT,
                _width,
                "height",
                G_TYPE_INT,
                _height,
                "alignment",
                G_TYPE_STRING,
                "au",
                "stream-format",
                G_TYPE_STRING,
                "byte-stream",
                nullptr));
            ELOG_DEBUG("H264 _caps[%p]", _caps.get())
        } else {
            _caps = gst::unique_from_ptr(gst_caps_new_simple(_mediaTypeCaps.c_str(),
                "width",
                G_TYPE_INT,
                _width,
                "height",
                G_TYPE_INT,
                _height,
                nullptr));
            ELOG_DEBUG("OTHER _caps[%p]", _caps.get())
        }
    }

    return _caps.get();
}
