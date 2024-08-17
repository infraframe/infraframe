

#ifndef VIDEO_ENCODER_FACTORY_H
#define VIDEO_ENCODER_FACTORY_H

#include "BufferPool.h"
#include "ClassMacro.h"
#include "EncoderPipeline.h"
#include "Helpers.h"

#include <api/video_codecs/video_encoder.h>
#include <common_video/include/video_frame_buffer_pool.h>
#include <media/base/codec.h>
#include <modules/video_coding/include/video_codec_interface.h>

#include <atomic>

namespace infraframe {
class GStreamerVideoEncoder : public webrtc::VideoEncoder {
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
    std::atomic<absl::optional<uint32_t>> _newBitRate;

public:
    GStreamerVideoEncoder(std::string mediaTypeCaps,
        std::string encoderPipeline,
        std::string encoderBitRatePropertyName,
        BitRateUnit encoderBitRatePropertyUnit,
        std::string encoderKeyframeIntervalPropertyName);
    ~GStreamerVideoEncoder() override = default;

    DECLARE_NOT_COPYABLE(GStreamerVideoEncoder);
    DECLARE_NOT_MOVABLE(GStreamerVideoEncoder);

    int32_t Release() override;

    int InitEncode(const webrtc::VideoCodec* codecSettings,
        const VideoEncoder::Settings& settings) override;

    int32_t RegisterEncodeCompleteCallback(
        webrtc::EncodedImageCallback* callback) override;

    int32_t
    Encode(const webrtc::VideoFrame& frame,
        const std::vector<webrtc::VideoFrameType>* frameTypes) override;

    void SetRates(const RateControlParameters& parameters) override;

    webrtc::VideoEncoder::EncoderInfo GetEncoderInfo() const override;

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
    webrtc::VideoFrameType
    getWebrtcFrameType(gst::unique_ptr<GstSample>& encodedSample);
};
} // namespace infraframe

#endif
