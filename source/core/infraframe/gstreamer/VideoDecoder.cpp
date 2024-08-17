#include "VideoDecoder.h"
#include "GstMappedFrame.h"
#include "OutPtr.h"

#include <libyuv.h>

#ifdef DEBUG_GSTREAMER
#include <iostream>
#endif

using namespace infraframe;
using namespace std;

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
    , _webrtcBufferPool { false, WebrtcBufferPoolSize }
{
}

int32_t GStreamerVideoDecoder::Release()
{
    if (_gstDecoderPipeline) {
        _gstDecoderPipeline.reset();
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoDecoder::Decode(const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    int64_t renderTimeMs)
{
    if (_keyframeNeeded) {
        if (inputImage._frameType != webrtc::VideoFrameType::kVideoFrameKey) {
            GST_WARNING(
                "Waiting for keyframe but got a delta unit... asking for keyframe");
            return WEBRTC_VIDEO_CODEC_ERROR;
        } else {
            initializeBufferTimestamps(renderTimeMs, inputImage.Timestamp());
            _keyframeNeeded = false;
        }
    }

    if (!_gstDecoderPipeline) {
        GST_ERROR("No source set, can't decode.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (_resetPipelineOnSizeChanges && inputImage._frameType == webrtc::VideoFrameType::kVideoFrameKey && _width != 0 && _height != 0 && (_width != inputImage._encodedWidth || _height != inputImage._encodedHeight)) {
        initializePipeline();
        initializeBufferTimestamps(renderTimeMs, inputImage.Timestamp());
    }

    auto sample = toGstSample(inputImage, renderTimeMs);
    if (!sample) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

#ifdef DEBUG_GSTREAMER
    GST_WARNING("Pushing sample: %" GST_PTR_FORMAT, sample.get());
    GST_WARNING("Width: %d, Height: %d, Size: %lu",
        _width,
        _height,
        gst_buffer_get_size(_buffer.get()));
#endif

    switch (_gstDecoderPipeline->pushSample(sample)) {
    case GST_FLOW_OK:
        break;
    case GST_FLOW_FLUSHING:
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    default:
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

#ifdef DEBUG_GSTREAMER
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(_gstAppPipeline->pipeline()),
        GST_DEBUG_GRAPH_SHOW_ALL,
        "pipeline-push-sample");
    cout << "Sample (push) is " << hex << sample.get() << dec << endl;
#endif

    return pullSample(renderTimeMs, inputImage.Timestamp(), inputImage.rotation_);
}

bool GStreamerVideoDecoder::Configure(
    const webrtc::VideoDecoder::Settings& settings)
{
    _keyframeNeeded = true;

    if (!initializePipeline()) {
        return false;
    }
    if (settings.buffer_pool_size().has_value()) {
        if (!_webrtcBufferPool.Resize(*settings.buffer_pool_size())) {
            return false;
        }
    }

    return _gstreamerBufferPool.initialize(GstDecoderBufferSize);
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
    if (inputImage.size() > gst_buffer_get_size(buffer.get())) {
        GST_ERROR("The buffer is too small");
        return nullptr;
    }

    gst_buffer_fill(buffer.get(), 0, inputImage.data(), inputImage.size());
    gst_buffer_set_size(buffer.get(), static_cast<gssize>(inputImage.size()));

    GST_BUFFER_DTS(buffer.get()) = (static_cast<guint64>(inputImage.Timestamp()) * GST_MSECOND) - _firstBufferDts;
    GST_BUFFER_PTS(buffer.get()) = (static_cast<guint64>(renderTimeMs) * GST_MSECOND) - _firstBufferPts;

    return gst::unique_from_ptr(gst_sample_new(
        buffer.get(), getCapsForFrame(inputImage), nullptr, nullptr));
}

int32_t GStreamerVideoDecoder::pullSample(int64_t renderTimeMs,
    uint32_t imageTimestamp,
    webrtc::VideoRotation rotation)
{
    GstState state;
    GstState pending;
    _gstDecoderPipeline->getSinkState(state, pending);

#ifdef DEBUG_GSTREAMER
    GST_ERROR("State: %s; Pending: %s",
        gst_element_state_get_name(state),
        gst_element_state_get_name(pending));

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(_gstAppPipeline->pipeline()),
        GST_DEBUG_GRAPH_SHOW_ALL,
        "pipeline-pull-sample");
#endif

    auto sample = _gstDecoderPipeline->tryPullSample();
    if (!sample) {
        if (!_gstDecoderPipeline->ready() || state != GST_STATE_PLAYING) {
            GST_ERROR("Not ready");
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        } else {
            GST_ERROR("Needs more data");
            return WEBRTC_VIDEO_CODEC_OK;
        }
    }

    _gstDecoderPipeline->setReady(true);

#ifdef DEBUG_GSTREAMER
    auto buffer = gst_sample_get_buffer(sample.get());

    GST_WARNING("Pulling sample: %" GST_PTR_FORMAT, sample.get());
    GST_WARNING("With size: %lu", gst_buffer_get_size(buffer));
    GstVideoInfo info;
    gst_video_info_from_caps(&info, gst_sample_get_caps(sample.get()));
    GST_VIDEO_INFO_FORMAT(&info);
#endif

    GstMappedFrame mappedFrame(sample.get(), GST_MAP_READ);
    if (!mappedFrame) {
        GST_ERROR("Could not map frame");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (mappedFrame.format() != GST_VIDEO_FORMAT_I420) {
        GST_ERROR("Wrong format: It must be I420");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    auto i420Buffer = _webrtcBufferPool.CreateI420Buffer(mappedFrame.width(),
        mappedFrame.height());
    if (!i420Buffer) {
        GST_ERROR("Could not create an I420 buffer");
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }

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

#ifdef DEBUG_GSTREAMER
    GST_LOG_OBJECT(_gstAppPipeline->pipeline(),
        "Output decoded frame! %d -> %" GST_PTR_FORMAT,
        frame.timestamp(),
        buffer);
#endif

    if (_imageReadyCb) {
        webrtc::VideoFrame decodedImage = webrtc::VideoFrame::Builder()
                                              .set_video_frame_buffer(i420Buffer)
                                              .set_timestamp_rtp(imageTimestamp)
                                              .set_timestamp_ms(renderTimeMs)
                                              .set_rotation(rotation)
                                              .build();
        _imageReadyCb->Decoded(decodedImage, absl::nullopt, absl::nullopt);
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

GstCaps*
GStreamerVideoDecoder::getCapsForFrame(const webrtc::EncodedImage& image)
{
    gint lastWidth = _width;
    gint lastHeight = _height;
    _width = image._encodedWidth != 0 ? image._encodedWidth : _width;
    _height = image._encodedHeight != 0 ? image._encodedHeight : _height;

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
        } else {
            _caps = gst::unique_from_ptr(gst_caps_new_simple(_mediaTypeCaps.c_str(),
                "width",
                G_TYPE_INT,
                _width,
                "height",
                G_TYPE_INT,
                _height,
                nullptr));
        }
    }

    return _caps.get();
}
