
#include "H264VideoEncoders.h"
#include "Support.h"

using namespace owt_base;
using namespace std;

DEFINE_LOGGER(H264GStreamerVideoEncoder, "owt.H264GStreamerVideoEncoder");

constexpr const char* BaselineProfileLevelIdPrefix = "42";
constexpr const char* MainProfileLevelIdPrefix = "4d";
constexpr const char* HighProfileLevelIdPrefix = "64";
constexpr const char* High444ProfileLevelIdPrefix = "f4";

H264GStreamerVideoEncoder::H264GStreamerVideoEncoder(
    const Parameters& parameters,
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
    const Parameters& parameters)
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
    const Parameters& parameters)
{
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);
    if (it == parameters.end()) {
        return true;
    } else {
        return it->second.find(BaselineProfileLevelIdPrefix) == 0 || it->second.find(MainProfileLevelIdPrefix) == 0;
    }
}

bool H264GStreamerVideoEncoder::isProfileBaselineOrMainOrHigh444(
    const Parameters& parameters)
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

NVH264GStreamerVideoEncoder::NVH264GStreamerVideoEncoder(
    const Parameters& parameters)
    : H264GStreamerVideoEncoder(
        parameters,
        "nvvideoconvert name=converter ! video/x-raw(memory:NVMM),format=I420 ! "
        "nvv4l2h264enc name=encoder profile="
            + profileFromParameters(parameters) + " ! h264parse",
        "bitrate", BitRateUnit::BitPerSec, "iframeinterval")
{
}

bool NVH264GStreamerVideoEncoder::isSupported()
{
    return gst::elementFactoryExists("nvvideoconvert") && gst::elementFactoryExists("nvv4l2h264enc") && gst::testEncoderDecoderPipeline("nvvideoconvert ! nvv4l2h264enc");
}

bool NVH264GStreamerVideoEncoder::isHardwareAccelerated() { return true; }

bool NVH264GStreamerVideoEncoder::areParametersSupported(
    const Parameters& parameters)
{
    return isProfileBaselineOrMain(parameters);
}

string NVH264GStreamerVideoEncoder::profileFromParameters(
    const Parameters& parameters)
{
    // It is using in SDP
    auto it = parameters.find(cricket::kH264FmtpProfileLevelId);

    if (it != parameters.end()) {
        if (it->second.find(MainProfileLevelIdPrefix) == 0) {
            return "2";
        } else if (it->second.find(HighProfileLevelIdPrefix) == 0) {
            return "4";
        } else if (it->second.find(High444ProfileLevelIdPrefix) == 0) {
            return "7";
        }
    }
    return "0";
}
