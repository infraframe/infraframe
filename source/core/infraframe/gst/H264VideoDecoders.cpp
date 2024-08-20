

#include "H264VideoDecoders.h"
#include "Support.h"

using namespace infraframe;
using namespace std;

DEFINE_LOGGER(H264GStreamerVideoDecoder, "infraframe.H264GStreamerVideoDecoder");

H264GStreamerVideoDecoder::H264GStreamerVideoDecoder(
    string decoderPipeline,
    bool resetPipelineOnSizeChanges)
    : GStreamerVideoDecoder(mediaTypeCaps(),
        std::move(decoderPipeline),
        resetPipelineOnSizeChanges)
{
}

const char* H264GStreamerVideoDecoder::mediaTypeCaps()
{
    return "video/x-h264";
}

const char* H264GStreamerVideoDecoder::codecName()
{
    return cricket::kH264CodecName;
}

NVH264GStreamerVideoDecoder::NVH264GStreamerVideoDecoder()
    : H264GStreamerVideoDecoder("nvv4l2decoder ! nvvideoconvert", true)
{
}

// webrtc::VideoDecoder::DecoderInfo
// NVH264GStreamerVideoDecoder::GetDecoderInfo() const
// {
//     webrtc::VideoDecoder::DecoderInfo info;
//     info.implementation_name = "GStreamer nvv4l2decoder h264";
//     info.is_hardware_accelerated = isHardwareAccelerated();
//     return info;
// }

bool NVH264GStreamerVideoDecoder::isSupported()
{
    return gst::elementFactoryExists("nvv4l2decoder") && gst::elementFactoryExists("nvvideoconvert") && gst::testEncoderDecoderPipeline("nvv4l2decoder ! nvvideoconvert");
}

bool NVH264GStreamerVideoDecoder::isHardwareAccelerated() { return true; }
