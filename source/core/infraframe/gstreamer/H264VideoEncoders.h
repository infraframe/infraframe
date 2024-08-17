

#ifndef H264_VIDEO_DECODER_FACTORY_H
#define H264_VIDEO_DECODER_FACTORY_H

#include "VideoEncoder.h"

namespace infraframe {
class H264GStreamerVideoEncoder : public GStreamerVideoEncoder {
    webrtc::H264PacketizationMode _packetizationMode;

public:
    H264GStreamerVideoEncoder(
        const webrtc::SdpVideoFormat::Parameters& parameters,
        std::string encoderPipeline,
        std::string encoderBitratePropertyName,
        BitRateUnit bitRatePropertyUnit,
        std::string keyframeIntervalPropertyName,
        const char* additionalMediaTypeCaps = "");
    ~H264GStreamerVideoEncoder() override = default;

    static std::string
    mediaTypeCaps(const webrtc::SdpVideoFormat::Parameters& parameters);
    static const char* codecName();

    static bool
    isProfileBaselineOrMain(const webrtc::SdpVideoFormat::Parameters& parameters);
    static bool isProfileBaselineOrMainOrHigh444(
        const webrtc::SdpVideoFormat::Parameters& parameters);

protected:
    int getKeyframeInterval(const webrtc::VideoCodec& codecSettings) override;

    void
    populateCodecSpecificInfo(webrtc::CodecSpecificInfo& codecSpecificInfo,
        const webrtc::EncodedImage& encodedFrame) override;
};

class NVENCH264GStreamerVideoEncoder : public H264GStreamerVideoEncoder {
public:
    explicit NVENCH264GStreamerVideoEncoder(
        const webrtc::SdpVideoFormat::Parameters& parameters);
    ~NVENCH264GStreamerVideoEncoder() override = default;

    [[nodiscard]] webrtc::VideoEncoder::EncoderInfo
    GetEncoderInfo() const override;

    static bool isSupported();
    static bool isHardwareAccelerated();
    static bool
    areParametersSupported(const webrtc::SdpVideoFormat::Parameters& parameters);

private:
    static std::string
    profileFromParameters(const webrtc::SdpVideoFormat::Parameters& parameters);
};

} // namespace infraframe

#endif
