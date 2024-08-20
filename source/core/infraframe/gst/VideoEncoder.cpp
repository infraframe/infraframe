#include "VideoEncoder.h"
#include "GstMappedBuffer.h"
#include "GstMappedFrame.h"

// #include <webrtc/modules/video_coding/utility/simulcast_utility.h>
#include <webrtc/api/video/i420_buffer.h>

#include <gst/video/video-info.h>

#include <libyuv.h>

#include <algorithm>
#include <new>

using namespace infraframe;
using namespace std;

DEFINE_LOGGER(GStreamerVideoEncoder, "infraframe.GStreamerVideoEncoder");

GStreamerVideoEncoder::GStreamerVideoEncoder(
    string mediaTypeCaps,
    string encoderPipeline,
    string encoderBitRatePropertyName,
    BitRateUnit encoderBitRatePropertyUnit,
    string encoderKeyframeIntervalPropertyName)
    : _mediaTypeCaps(std::move(mediaTypeCaps))
    , _encoderPipeline(std::move(encoderPipeline))
    , _encoderBitRatePropertyName(std::move(encoderBitRatePropertyName))
    , _encoderBitRatePropertyUnit(encoderBitRatePropertyUnit)
    , _encoderKeyframeIntervalPropertyName(
          std::move(encoderKeyframeIntervalPropertyName))
    , _firstBufferPts { GST_CLOCK_TIME_NONE }
    , _firstBufferDts { GST_CLOCK_TIME_NONE }
    , _imageReadyCb { nullptr }
    , _dropNextFrame(false)
{
}

int32_t GStreamerVideoEncoder::Release()
{
    if (_gstEncoderPipeline) {
        _gstEncoderPipeline.reset();
    }
    if (_encodedFrame._buffer) {
        delete[] _encodedFrame._buffer;
        _encodedFrame._buffer = nullptr;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}
// int GStreamerVideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings,
//     const VideoEncoder::Settings& settings)
int32_t GStreamerVideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, int32_t number_of_cores, size_t max_payload_size)
{
    ELOG_DEBUG("%d, %d, %d, %d, %d", codecSettings->startBitrate,
        codecSettings->maxBitrate,
        codecSettings->maxFramerate,
        codecSettings->width,
        codecSettings->height);
    if (codecSettings == nullptr || codecSettings->maxFramerate < 1 || (codecSettings->maxBitrate > 0 && codecSettings->startBitrate > codecSettings->maxBitrate) || codecSettings->width < 1 || codecSettings->height < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    ELOG_DEBUG("2");
    // if (webrtc::SimulcastUtility::NumberOfSimulcastStreams(*codecSettings) > 1) {
    //     ELOG_ERROR("Simulcast not supported.");
    //     return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
    // }

    ELOG_DEBUG("3");
    // GST_INFO("Initializing encoder (%s)", GetEncoderInfo().implementation_name.c_str());
    if (!initializePipeline()) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    ELOG_DEBUG("4");
    _gstEncoderPipeline->setBitRate(codecSettings->startBitrate * 1000);
    _gstEncoderPipeline->setKeyframeInterval(getKeyframeInterval(*codecSettings));

    ELOG_DEBUG("5");
    _inputVideoInfo = gst::unique_from_ptr(gst_video_info_new());
    ELOG_DEBUG("6");
    gst_video_info_set_format(_inputVideoInfo.get(),
        GST_VIDEO_FORMAT_I420,
        codecSettings->width,
        codecSettings->height);
    ELOG_DEBUG("7");

    _inputVideoCaps = gst::unique_from_ptr(
        gst_video_info_to_caps(_inputVideoInfo.get()));

    ELOG_DEBUG("8");
    if (!_gstreamerBufferPool.initialize(GST_VIDEO_INFO_SIZE(_inputVideoInfo))) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ELOG_DEBUG("9");
    // _encodedFrame.SetEncodedData(
    //     webrtc::EncodedImageBuffer::Create(GST_VIDEO_INFO_SIZE(_inputVideoInfo)));

    auto newSize = GST_VIDEO_INFO_SIZE(_inputVideoInfo);
    _encodedFrame._buffer = new uint8_t[newSize];
    _encodedFrame._size = newSize;

    _encodedFrame._encodedWidth = codecSettings->width;
    _encodedFrame._encodedHeight = codecSettings->height;

    ELOG_DEBUG("10");
    return WEBRTC_VIDEO_CODEC_OK;
}

// int32_t GStreamerVideoEncoder::InitEncode(const webrtc::VideoCodec* codec_settings, int32_t number_of_cores, size_t max_payload_size)
// {
//     VideoEncoder::Capabilities kCapabilities(false);
//     VideoEncoder::Settings settings = VideoEncoder::Settings(kCapabilities, 1, 1200);
//     return InitEncode(codec_settings, settings);
// }

int32_t GStreamerVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback)
{
    _imageReadyCb = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}
// int32_t GStreamerVideoEncoder::Encode(
//     const webrtc::VideoFrame& frame,
//     const vector<webrtc::VideoFrameType>* frameTypes)
int32_t GStreamerVideoEncoder::Encode(const webrtc::VideoFrame& frame,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    const std::vector<webrtc::FrameType>* frameTypes)
{
    ELOG_DEBUG("GStreamerVideoEncoder::Encode");
    if (!_imageReadyCb) {
        ELOG_ERROR("The encoded image callback is not set.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    ELOG_DEBUG("!_gstEncoderPipeline");
    if (!_gstEncoderPipeline) {
        ELOG_ERROR("The pipeline is not created.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    ELOG_DEBUG("!GST_CLOCK_TIME_IS_VALID(_firstBufferPts) || !GST_CLOCK_TIME_IS_VALID(_firstBufferDts)");
    if (!GST_CLOCK_TIME_IS_VALID(_firstBufferPts) || !GST_CLOCK_TIME_IS_VALID(_firstBufferDts)) {
        ELOG_DEBUG("initializeBufferTimestamps");
        initializeBufferTimestamps(frame.render_time_ms(), frame.timestamp());
    }

    if (frameTypes) {
        ELOG_DEBUG("keyFrameNeeded, frameTypes.size() [%d]", frameTypes->size());
        bool keyFrameNeeded = any_of(frameTypes->begin(),
            frameTypes->end(),
            [](const webrtc::FrameType& t) {
                return t == webrtc::FrameType::kVideoFrameKey;
            });
        ELOG_DEBUG("keyFrameNeeded: [%s]", keyFrameNeeded ? "TRUE" : "FALSE");
        if (keyFrameNeeded) {
            ELOG_DEBUG("_gstEncoderPipeline->forceKeyFrame()");
            _gstEncoderPipeline->forceKeyFrame();
        }
    }
    ELOG_DEBUG("_dropNextFrame: [%s]", _dropNextFrame ? "TRUE" : "FALSE");
    if (_dropNextFrame) {
        _dropNextFrame = false;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    // auto newBitRate = _newBitRate.swap(std::nullopt);
    // if (newBitRate.has_value()) {
    //     _gstEncoderPipeline->setBitRate(*newBitRate);
    // }

    ELOG_DEBUG("toGstSample");
    auto sample = toGstSample(frame);
    if (!sample) {
        ELOG_DEBUG("NO sample");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    ELOG_DEBUG("_gstEncoderPipeline->pushSample(sample)");
    _gstEncoderPipeline->pushSample(sample);

    ELOG_DEBUG("_gstEncoderPipeline->tryPullSample()");
    auto encodedSample = _gstEncoderPipeline->tryPullSample();
    if (!encodedSample) {
        ELOG_ERROR("No encoded sample available");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    ELOG_DEBUG("updateEncodedFrame(frame, encodedSample)");
    if (!updateEncodedFrame(frame, encodedSample)) {
        ELOG_DEBUG("updateEncodedFrame ERROR");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // webrtc::CodecSpecificInfo codecSpecificInfo;
    // populateCodecSpecificInfo(codecSpecificInfo, _encodedFrame);
    // auto result = _imageReadyCb->OnEncodedImage(_encodedFrame, &codecSpecificInfo);
    ELOG_DEBUG("_imageReadyCb->OnEncodedImage [BEGIN]");
    auto result = _imageReadyCb->OnEncodedImage(_encodedFrame, nullptr, nullptr);
    if (result.error != webrtc::EncodedImageCallback::Result::OK) {
        ELOG_ERROR("OnEncodedImage ERROR");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    ELOG_DEBUG("_imageReadyCb->OnEncodedImage [END]");
    _dropNextFrame = result.drop_next_frame;

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoEncoder::SetChannelParameters(uint32_t packet_loss, int64_t rtt)
{
    return WEBRTC_VIDEO_CODEC_OK;
};

// void GStreamerVideoEncoder::SetRates(const RateControlParameters& parameters)
// {
//     _newBitRate.store(parameters.bitrate.get_sum_bps());
// }

// webrtc::VideoEncoder::EncoderInfo
// GStreamerVideoEncoder::GetEncoderInfo() const
// {
//     webrtc::VideoEncoder::EncoderInfo info;
//     info.requested_resolution_alignment = 2;
//     info.supports_simulcast = false;
//     info.preferred_pixel_formats = { webrtc::VideoFrameBuffer::Type::kI420 };

//     return info;
// }

bool GStreamerVideoEncoder::initializePipeline()
{
    _gstEncoderPipeline = make_unique<GStreamerEncoderPipeline>();
    ELOG_DEBUG("[%s] [%d] [%s] [%s] [%s]", _encoderBitRatePropertyName.c_str(),
        int(_encoderBitRatePropertyUnit),
        _encoderKeyframeIntervalPropertyName.c_str(),
        _mediaTypeCaps.c_str(),
        _encoderPipeline.c_str());
    return _gstEncoderPipeline->initialize(_encoderBitRatePropertyName,
               _encoderBitRatePropertyUnit,
               _encoderKeyframeIntervalPropertyName,
               _mediaTypeCaps,
               _encoderPipeline)
        == WEBRTC_VIDEO_CODEC_OK;
}

void GStreamerVideoEncoder::initializeBufferTimestamps(
    int64_t renderTimeMs,
    uint32_t imageTimestamp)
{
    _firstBufferPts = (static_cast<guint64>(renderTimeMs)) * GST_MSECOND;
    _firstBufferDts = (static_cast<guint64>(imageTimestamp)) * GST_MSECOND;
    ELOG_DEBUG("_firstBufferPts[%llu], _firstBufferDts[%llu]", _firstBufferPts, _firstBufferDts);
}

gst::unique_ptr<GstSample>
GStreamerVideoEncoder::toGstSample(const webrtc::VideoFrame& frame)
{
    gst::unique_ptr<GstBuffer> buffer = _gstreamerBufferPool.acquireBuffer();
    if (!buffer) {
        ELOG_ERROR("No buffer available");
        return nullptr;
    }

    // auto i420Buffer = frame.video_frame_buffer()->ToI420();
    auto i420Buffer = webrtc::I420Buffer::Copy(*frame.video_frame_buffer());
    if (GST_VIDEO_INFO_WIDTH(_inputVideoInfo.get()) != i420Buffer->width() || GST_VIDEO_INFO_HEIGHT(_inputVideoInfo.get()) != i420Buffer->height()) {
        ELOG_ERROR("The input frame size is invalid");
        return nullptr;
    }

    GstMappedFrame mappedFrame(
        buffer.get(), _inputVideoInfo.get(), GST_MAP_WRITE);
    if (!mappedFrame) {
        ELOG_ERROR("Could not map frame");
        return nullptr;
    }

    libyuv::I420Copy(i420Buffer->DataY(),
        i420Buffer->StrideY(),
        i420Buffer->DataU(),
        i420Buffer->StrideU(),
        i420Buffer->DataV(),
        i420Buffer->StrideV(),
        mappedFrame.componentData(0),
        mappedFrame.componentStride(0),
        mappedFrame.componentData(1),
        mappedFrame.componentStride(1),
        mappedFrame.componentData(2),
        mappedFrame.componentStride(2),
        i420Buffer->width(),
        i420Buffer->height());

    GST_BUFFER_DTS(buffer.get()) = (static_cast<guint64>(frame.timestamp()) * GST_MSECOND) - _firstBufferDts;
    GST_BUFFER_PTS(buffer.get()) = (static_cast<guint64>(frame.render_time_ms()) * GST_MSECOND) - _firstBufferPts;

    return gst::unique_from_ptr(
        gst_sample_new(buffer.get(), _inputVideoCaps.get(), nullptr, nullptr));
}

bool GStreamerVideoEncoder::updateEncodedFrame(
    const webrtc::VideoFrame& frame,
    gst::unique_ptr<GstSample>& encodedSample)
{
    GstMappedBuffer mappedBuffer(gst_sample_get_buffer(encodedSample.get()),
        GST_MAP_READ);
    if (!mappedBuffer) {
        ELOG_ERROR("gst_buffer_map failed");
        return false;
    }

    if (_encodedFrame._size < mappedBuffer.size()) {
        // _encodedFrame.SetEncodedData(webrtc::EncodedImageBuffer::Create(
        //     2 * _encodedFrame.GetEncodedData()->size()));
        ELOG_DEBUG("updateEncodedFrame BUFFER");
        delete[] _encodedFrame._buffer;
        auto newSize = 2 * _encodedFrame._size;
        _encodedFrame._buffer = new uint8_t[newSize];
        _encodedFrame._size = newSize;
    }

    memcpy(_encodedFrame._buffer, mappedBuffer.data(), mappedBuffer.size());

    _encodedFrame._length = mappedBuffer.size();
    _encodedFrame._frameType = getWebrtcFrameType(encodedSample);
    _encodedFrame.capture_time_ms_ = frame.render_time_ms();
    _encodedFrame._timeStamp = frame.timestamp();
    _encodedFrame.ntp_time_ms_ = frame.ntp_time_ms();
    _encodedFrame.rotation_ = frame.rotation();

    return true;
}

webrtc::FrameType GStreamerVideoEncoder::getWebrtcFrameType(
    gst::unique_ptr<GstSample>& encodedSample)
{
    if (GST_BUFFER_FLAG_IS_SET(gst_sample_get_buffer(encodedSample.get()),
            GST_BUFFER_FLAG_DELTA_UNIT)) {
        return webrtc::FrameType::kVideoFrameDelta;
    } else {
        return webrtc::FrameType::kVideoFrameKey;
    }
}
