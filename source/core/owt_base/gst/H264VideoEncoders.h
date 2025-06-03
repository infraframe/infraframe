

#ifndef H264_VIDEO_ENCODERS_H
#define H264_VIDEO_ENCODERS_H

#include "VideoEncoder.h"

#include <logger.h>

#include <map>
#include <string>

using Parameters = std::map<std::string, std::string>;
namespace owt_base {
class H264GStreamerVideoEncoder : public GStreamerVideoEncoder {
    DECLARE_LOGGER();
    webrtc::H264PacketizationMode _packetizationMode;

public:
    H264GStreamerVideoEncoder(
        const Parameters& parameters,
        std::string encoderPipeline,
        std::string encoderBitratePropertyName,
        BitRateUnit bitRatePropertyUnit,
        std::string keyframeIntervalPropertyName,
        const char* additionalMediaTypeCaps = "");
    ~H264GStreamerVideoEncoder() override = default;

    static std::string
    mediaTypeCaps(const Parameters& parameters);
    static const char* codecName();

    static bool
    isProfileBaselineOrMain(const Parameters& parameters);
    static bool isProfileBaselineOrMainOrHigh444(
        const Parameters& parameters);

protected:
    int getKeyframeInterval(const webrtc::VideoCodec& codecSettings) override;

    void
    populateCodecSpecificInfo(webrtc::CodecSpecificInfo& codecSpecificInfo,
        const webrtc::EncodedImage& encodedFrame) override;
};

class NVH264GStreamerVideoEncoder : public H264GStreamerVideoEncoder {
public:
    explicit NVH264GStreamerVideoEncoder(
        const Parameters& parameters);
    ~NVH264GStreamerVideoEncoder() override = default;

    static bool isSupported();
    static bool isHardwareAccelerated();
    static bool
    areParametersSupported(const Parameters& parameters);

private:
    static std::string
    profileFromParameters(const Parameters& parameters);
};

} // namespace owt_base

#endif
