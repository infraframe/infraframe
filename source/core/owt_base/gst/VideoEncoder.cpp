#include "VideoEncoder.h"
#include "GstMappedBuffer.h"
#include "GstMappedFrame.h"

#include <webrtc/api/video/i420_buffer.h>

#include <gst/video/video-info.h>

#include <libyuv.h>

#include <algorithm>
#include <new>

using namespace owt_base;
using namespace std;

DEFINE_LOGGER(GStreamerVideoEncoder, "owt.GStreamerVideoEncoder");

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
    // if (webrtc::SimulcastUtility::NumberOfSimulcastStreams(*codecSettings) > 1) {
    //     ELOG_ERROR("Simulcast not supported.");
    //     return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
    // }

    if (!initializePipeline()) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _gstEncoderPipeline->setBitRate(codecSettings->startBitrate * 1000);
    _gstEncoderPipeline->setKeyframeInterval(getKeyframeInterval(*codecSettings));
    _gstEncoderPipeline->setResolution(codecSettings->width, codecSettings->height);

    _inputVideoInfo = gst::unique_from_ptr(gst_video_info_new());
    gst_video_info_set_format(_inputVideoInfo.get(),
        GST_VIDEO_FORMAT_I420,
        codecSettings->width,
        codecSettings->height);

    _inputVideoCaps = gst::unique_from_ptr(gst_video_info_to_caps(_inputVideoInfo.get()));

    if (!_gstreamerBufferPool.initialize(GST_VIDEO_INFO_SIZE(_inputVideoInfo))) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    auto newSize = GST_VIDEO_INFO_SIZE(_inputVideoInfo);
    _encodedFrame._buffer = new uint8_t[newSize];
    _encodedFrame._size = newSize;

    _encodedFrame._encodedWidth = codecSettings->width;
    _encodedFrame._encodedHeight = codecSettings->height;

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback)
{
    _imageReadyCb = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoEncoder::Encode(const webrtc::VideoFrame& frame,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    const std::vector<webrtc::FrameType>* frameTypes)
{
    if (!_imageReadyCb) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!_gstEncoderPipeline) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (!GST_CLOCK_TIME_IS_VALID(_firstBufferPts) || !GST_CLOCK_TIME_IS_VALID(_firstBufferDts)) {
        initializeBufferTimestamps(frame.render_time_ms(), frame.timestamp());
    }

    if (frameTypes) {
        bool keyFrameNeeded = any_of(frameTypes->begin(),
            frameTypes->end(),
            [](const webrtc::FrameType& t) {
                return t == webrtc::FrameType::kVideoFrameKey;
            });
        if (keyFrameNeeded) {
            _gstEncoderPipeline->forceKeyFrame();
        }
    }
    if (_dropNextFrame) {
        _dropNextFrame = false;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    auto newBitRate = _newBitRate.exchange(std::nullopt);
    if (newBitRate.has_value()) {
        _gstEncoderPipeline->setBitRate(*newBitRate);
    }

    auto sample = toGstSample(frame);
    if (!sample) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _gstEncoderPipeline->pushSample(sample);

    auto encodedSample = _gstEncoderPipeline->tryPullSample();
    if (!encodedSample) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    if (!updateEncodedFrame(frame, encodedSample)) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    auto result = _imageReadyCb->OnEncodedImage(_encodedFrame, nullptr, nullptr);
    if (result.error != webrtc::EncodedImageCallback::Result::OK) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _dropNextFrame = result.drop_next_frame;

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoEncoder::SetChannelParameters(uint32_t packet_loss, int64_t rtt)
{
    return WEBRTC_VIDEO_CODEC_OK;
};

int32_t GStreamerVideoEncoder::SetRates(uint32_t bitrate, uint32_t framerate)
{
    _newBitRate.store(bitrate);
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t GStreamerVideoEncoder::SetResolution(uint32_t width, uint32_t height)
{
    _inputVideoInfo = gst::unique_from_ptr(gst_video_info_new());
    gst_video_info_set_format(_inputVideoInfo.get(), GST_VIDEO_FORMAT_I420, width, height);
    _inputVideoCaps = gst::unique_from_ptr(gst_video_info_to_caps(_inputVideoInfo.get()));

    if (!_gstreamerBufferPool.initialize(GST_VIDEO_INFO_SIZE(_inputVideoInfo))) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    auto newSize = GST_VIDEO_INFO_SIZE(_inputVideoInfo);
    delete[] _encodedFrame._buffer;
    _encodedFrame._buffer = new uint8_t[newSize];
    _encodedFrame._size = newSize;
    _encodedFrame._encodedWidth = width;
    _encodedFrame._encodedHeight = height;

    _gstEncoderPipeline->setResolution(width, height);
    return WEBRTC_VIDEO_CODEC_OK;
}

bool GStreamerVideoEncoder::initializePipeline()
{
    _gstEncoderPipeline = make_unique<GStreamerEncoderPipeline>();
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
}

gst::unique_ptr<GstSample>
GStreamerVideoEncoder::toGstSample(const webrtc::VideoFrame& frame)
{
    gst::unique_ptr<GstBuffer> buffer = _gstreamerBufferPool.acquireBuffer();
    if (!buffer) {
        ELOG_ERROR("No buffer available");
        return nullptr;
    }

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
        return false;
    }

    if (_encodedFrame._size < mappedBuffer.size()) {
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
