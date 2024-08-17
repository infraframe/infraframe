
#include "H264VideoEncoders.h"
#include "Support.h"

using namespace infraframe;
using namespace std;

constexpr const char* BaselineProfileLevelIdPrefix = "42";
constexpr const char* MainProfileLevelIdPrefix = "4f";
constexpr const char* High444ProfileLevelIdPrefix = "f4";

H264GStreamerVideoEncoder::H264GStreamerVideoEncoder(
    const webrtc::SdpVideoFormat::Parameters& parameters,
    string encoderPipeline,
    string encoderBitratePropertyName,
    BitRateUnit bitRatePropertyUnit,
    string keyframeIntervalPropertyName,
    const char* additionalMediaTypeCaps)
    : GStreamerVideoEncoder(mediaTypeCaps(parameters) + additionalMediaTypeCaps,
        std::move(encoderPipeline),
        std::move(encoderBitratePropertyName),
        bitRatePropertyUnit,
        std::move(keyframeIntervalPropertyName))
{
    auto packetizationModeIt = parameters.find(
        cricket::kH264FmtpPacketizationMode);
    if (packetizationModeIt == parameters.end() || packetizationModeIt->second == "0") {
        _packetizationMode = webrtc::H264PacketizationMode::SingleNalUnit;
    } else {
        _packetizationMode = webrtc::H264PacketizationMode::NonInterleaved;
    }
}

string H264GStreamerVideoEncoder::mediaTypeCaps(
    const webrtc::SdpVideoFormat::Parameters& parameters)
{
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);
    if (it == parameters.end() || it->second.find(BaselineProfileLevelIdPrefix) == 0) {
        return "video/"
               "x-h264,alignment=au,stream-format=byte-stream,profile=baseline";
    } else if (it->second.find(MainProfileLevelIdPrefix) == 0) {
        return "video/x-h264,alignment=au,stream-format=byte-stream,profile=main";
    } else if (it->second.find(High444ProfileLevelIdPrefix) == 0) {
        return "video/"
               "x-h264,alignment=au,stream-format=byte-stream,profile=high-4:4:4";
    } else {
        return "video/x-h264,alignment=au,stream-format=byte-stream";
    }
}

const char* H264GStreamerVideoEncoder::codecName()
{
    return cricket::kH264CodecName;
}

bool H264GStreamerVideoEncoder::isProfileBaselineOrMain(
    const webrtc::SdpVideoFormat::Parameters& parameters)
{
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);
    if (it == parameters.end()) {
        return true;
    } else {
        return it->second.find(BaselineProfileLevelIdPrefix) == 0 || it->second.find(MainProfileLevelIdPrefix) == 0;
    }
}

bool H264GStreamerVideoEncoder::isProfileBaselineOrMainOrHigh444(
    const webrtc::SdpVideoFormat::Parameters& parameters)
{
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);
    if (it == parameters.end()) {
        return true;
    } else {
        return it->second.find(BaselineProfileLevelIdPrefix) == 0 || it->second.find(MainProfileLevelIdPrefix) == 0 || it->second.find(High444ProfileLevelIdPrefix) == 0;
    }
}

int H264GStreamerVideoEncoder::getKeyframeInterval(
    const webrtc::VideoCodec& codecSettings)
{
    return codecSettings.H264().keyFrameInterval;
}

void H264GStreamerVideoEncoder::populateCodecSpecificInfo(
    webrtc::CodecSpecificInfo& codecSpecificInfo,
    const webrtc::EncodedImage& encodedFrame)
{
    codecSpecificInfo.codecType = webrtc::kVideoCodecH264;
    codecSpecificInfo.codecSpecific.H264.packetization_mode = _packetizationMode;
}

NVENCH264GStreamerVideoEncoder::NVENCH264GStreamerVideoEncoder(
    const webrtc::SdpVideoFormat::Parameters& parameters)
    : H264GStreamerVideoEncoder(
        parameters,
        "nvvideoconvert ! 'video/x-raw(memory:NVMM),format=I420' ! "
        "nvv4l2h264enc name=encoder profile="
            + profileFromParameters(parameters) + " ! h264parse",
        "bitrate",
        BitRateUnit::BitPerSec,
        "iframeinterval")
{
}

webrtc::VideoEncoder::EncoderInfo
NVENCH264GStreamerVideoEncoder::GetEncoderInfo() const
{
    webrtc::VideoEncoder::EncoderInfo info(
        GStreamerVideoEncoder::GetEncoderInfo());
    info.implementation_name = "GStreamer - nvv4l2h264enc";
    info.is_hardware_accelerated = false;

    return info;
}

bool NVENCH264GStreamerVideoEncoder::isSupported()
{
    return gst::elementFactoryExists("nvvideoconvert") && gst::elementFactoryExists("nvv4l2h264enc") && gst::testEncoderDecoderPipeline("nvvideoconvert ! nvv4l2h264enc");
}

bool NVENCH264GStreamerVideoEncoder::isHardwareAccelerated() { return true; }

bool NVENCH264GStreamerVideoEncoder::areParametersSupported(
    const webrtc::SdpVideoFormat::Parameters& parameters)
{
    return isProfileBaselineOrMain(parameters);
}

string NVENCH264GStreamerVideoEncoder::profileFromParameters(
    const webrtc::SdpVideoFormat::Parameters& parameters)
{
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);
    if (it != parameters.end() && it->second.find(MainProfileLevelIdPrefix) == 0) {
        return "2";
    } else {
        return "0";
    }
}
