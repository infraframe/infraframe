

#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include "BufferPool.h"
#include "ClassMacro.h"
#include "EncoderPipeline.h"
#include "Helpers.h"

#include <I420BufferManager.h>
#include <logger.h>

#include <webrtc/api/video_codecs/video_encoder.h>
#include <webrtc/media/base/codec.h>
#include <webrtc/modules/video_coding/include/video_codec_interface.h>

#include <atomic>
#include <optional>

#include <boost/scoped_ptr.hpp>

namespace owt_base {
class GStreamerVideoEncoder : public webrtc::VideoEncoder {
    DECLARE_LOGGER();
    std::string _mediaTypeCaps;
    std::string _encoderPipeline;
    std::string _encoderBitRatePropertyName;
    BitRateUnit _encoderBitRatePropertyUnit;
    std::string _encoderKeyframeIntervalPropertyName;

    std::unique_ptr<GStreamerEncoderPipeline> _gstEncoderPipeline;
    GStreamerBufferPool _gstreamerBufferPool;

    gst::unique_ptr<GstVideoInfo> _inputVideoInfo;
    gst::unique_ptr<GstCaps> _inputVideoCaps;

    GstClockTime _firstBufferPts;
    GstClockTime _firstBufferDts;

    webrtc::EncodedImage _encodedFrame;
    webrtc::EncodedImageCallback* _imageReadyCb;

    bool _dropNextFrame;
    std::atomic<std::optional<uint32_t>> _newBitRate;

public:
    GStreamerVideoEncoder(std::string mediaTypeCaps,
        std::string encoderPipeline,
        std::string encoderBitRatePropertyName,
        BitRateUnit encoderBitRatePropertyUnit,
        std::string encoderKeyframeIntervalPropertyName);
    ~GStreamerVideoEncoder() override = default;

    DECLARE_NOT_COPYABLE(GStreamerVideoEncoder);
    DECLARE_NOT_MOVABLE(GStreamerVideoEncoder);

    int32_t InitEncode(const webrtc::VideoCodec* codec_settings, int32_t number_of_cores, size_t max_payload_size) override;

    int32_t RegisterEncodeCompleteCallback(
        webrtc::EncodedImageCallback* callback) override;

    int32_t Release() override;

    int32_t Encode(const webrtc::VideoFrame& frame,
        const webrtc::CodecSpecificInfo* codec_specific_info,
        const std::vector<webrtc::FrameType>* frame_types) override;

    int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) override;

    int32_t SetRates(uint32_t bitrate, uint32_t framerate) override;

    int32_t SetResolution(uint32_t width, uint32_t height) override;

protected:
    virtual int getKeyframeInterval(const webrtc::VideoCodec& codecSettings) = 0;

    virtual void
    populateCodecSpecificInfo(webrtc::CodecSpecificInfo& codecSpecificInfo,
        const webrtc::EncodedImage& encodedFrame)
        = 0;

private:
    bool initializePipeline();
    void initializeBufferTimestamps(int64_t renderTimeMs,
        uint32_t imageTimestamp);

    gst::unique_ptr<GstSample> toGstSample(const webrtc::VideoFrame& frame);
    bool updateEncodedFrame(const webrtc::VideoFrame& frame,
        gst::unique_ptr<GstSample>& encodedSample);
    webrtc::FrameType
    getWebrtcFrameType(gst::unique_ptr<GstSample>& encodedSample);
};
} // namespace owt_base

#endif
