#include "VideoEncoder.h"
#include "GstMappedBuffer.h"
#include "GstMappedFrame.h"

#include <modules/video_coding/utility/simulcast_utility.h>

#include <libyuv.h>

using namespace infraframe;
using namespace std;

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
    return WEBRTC_VIDEO_CODEC_OK;
}

int GStreamerVideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings,
    const VideoEncoder::Settings& settings)
{
    if (codecSettings == nullptr || codecSettings->maxFramerate < 1 || (codecSettings->maxBitrate > 0 && codecSettings->startBitrate > codecSettings->maxBitrate) || codecSettings->width < 1 || codecSettings->height < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (webrtc::SimulcastUtility::NumberOfSimulcastStreams(*codecSettings) > 1) {
        GST_ERROR("Simulcast not supported.");
        return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
    }

    GST_INFO("Initializing encoder (%s)",
        GetEncoderInfo().implementation_name.c_str());
    if (!initializePipeline()) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _gstEncoderPipeline->setBitRate(codecSettings->startBitrate * 1000);
    _gstEncoderPipeline->setKeyframeInterval(getKeyframeInterval(*codecSettings));

    _inputVideoInfo = gst::unique_from_ptr(gst_video_info_new());
    gst_video_info_set_format(_inputVideoInfo.get(),
        GST_VIDEO_FORMAT_I420,
        codecSettings->width,
        codecSettings->height);

    _inputVideoCaps = gst::unique_from_ptr(
        gst_video_info_to_caps(_inputVideoInfo.get()));

    if (!_gstreamerBufferPool.initialize(GST_VIDEO_INFO_SIZE(_inputVideoInfo))) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _encodedFrame.SetEncodedData(
        webrtc::EncodedImageBuffer::Create(GST_VIDEO_INFO_SIZE(_inputVideoInfo)));
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

int32_t GStreamerVideoEncoder::Encode(
    const webrtc::VideoFrame& frame,
    const vector<webrtc::VideoFrameType>* frameTypes)
{
    if (!_imageReadyCb) {
        GST_ERROR("The encoded image callback is not set.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!_gstEncoderPipeline) {
        GST_ERROR("The pipeline is not created.");
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (!GST_CLOCK_TIME_IS_VALID(_firstBufferPts) || !GST_CLOCK_TIME_IS_VALID(_firstBufferDts)) {
        initializeBufferTimestamps(frame.render_time_ms(), frame.timestamp());
    }

    bool keyFrameNeeded = any_of(frameTypes->begin(),
        frameTypes->end(),
        [](const webrtc::VideoFrameType& t) {
            return t == webrtc::VideoFrameType::kVideoFrameKey;
        });
    if (keyFrameNeeded) {
        _gstEncoderPipeline->forceKeyFrame();
    }
    if (_dropNextFrame) {
        _dropNextFrame = false;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    auto newBitRate = _newBitRate.exchange(absl::nullopt);
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
        GST_ERROR("No encoded sample available");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    if (!updateEncodedFrame(frame, encodedSample)) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    webrtc::CodecSpecificInfo codecSpecificInfo;
    populateCodecSpecificInfo(codecSpecificInfo, _encodedFrame);

    auto result = _imageReadyCb->OnEncodedImage(_encodedFrame,
        &codecSpecificInfo);
    if (result.error != webrtc::EncodedImageCallback::Result::OK) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _dropNextFrame = result.drop_next_frame;

    return WEBRTC_VIDEO_CODEC_OK;
}

void GStreamerVideoEncoder::SetRates(const RateControlParameters& parameters)
{
    _newBitRate.store(parameters.bitrate.get_sum_bps());
}

webrtc::VideoEncoder::EncoderInfo
GStreamerVideoEncoder::GetEncoderInfo() const
{
    webrtc::VideoEncoder::EncoderInfo info;
    info.requested_resolution_alignment = 2;
    info.supports_simulcast = false;
    info.preferred_pixel_formats = { webrtc::VideoFrameBuffer::Type::kI420 };

    return info;
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
        GST_ERROR("No buffer available");
        return nullptr;
    }

    auto i420Buffer = frame.video_frame_buffer()->ToI420();
    if (GST_VIDEO_INFO_WIDTH(_inputVideoInfo.get()) != i420Buffer->width() || GST_VIDEO_INFO_HEIGHT(_inputVideoInfo.get()) != i420Buffer->height()) {
        GST_ERROR("The input frame size is invalid");
        return nullptr;
    }

    GstMappedFrame mappedFrame(
        buffer.get(), _inputVideoInfo.get(), GST_MAP_WRITE);
    if (!mappedFrame) {
        GST_ERROR("Could not map frame");
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
        GST_ERROR("gst_buffer_map failed");
        return false;
    }

    if (_encodedFrame.GetEncodedData()->size() < mappedBuffer.size()) {
        _encodedFrame.SetEncodedData(webrtc::EncodedImageBuffer::Create(
            2 * _encodedFrame.GetEncodedData()->size()));
    }

    memcpy(_encodedFrame.GetEncodedData()->data(),
        mappedBuffer.data(),
        mappedBuffer.size());

    _encodedFrame.set_size(mappedBuffer.size());
    _encodedFrame._frameType = getWebrtcFrameType(encodedSample);
    _encodedFrame.capture_time_ms_ = frame.render_time_ms();
    _encodedFrame.SetTimestamp(frame.timestamp());
    _encodedFrame.ntp_time_ms_ = frame.ntp_time_ms();
    _encodedFrame.rotation_ = frame.rotation();

    return true;
}

webrtc::VideoFrameType GStreamerVideoEncoder::getWebrtcFrameType(
    gst::unique_ptr<GstSample>& encodedSample)
{
    if (GST_BUFFER_FLAG_IS_SET(gst_sample_get_buffer(encodedSample.get()),
            GST_BUFFER_FLAG_DELTA_UNIT)) {
        return webrtc::VideoFrameType::kVideoFrameDelta;
    } else {
        return webrtc::VideoFrameType::kVideoFrameKey;
    }
}
