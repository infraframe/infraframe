

#include "H265VideoDecoders.h"
#include "Support.h"

using namespace infraframe;
using namespace std;

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

NVENCH265GStreamerVideoDecoder::NVENCH265GStreamerVideoDecoder()
    : H264GStreamerVideoDecoder("nvv4l2decoder ! nvvideoconvert", true)
{
}

webrtc::VideoDecoder::DecoderInfo
NVENCH265GStreamerVideoDecoder::GetDecoderInfo() const
{
    webrtc::VideoDecoder::DecoderInfo info;
    info.implementation_name = "GStreamer nvv4l2decoder h264";
    info.is_hardware_accelerated = isHardwareAccelerated();
    return info;
}

bool NVENCH265GStreamerVideoDecoder::isSupported()
{
    return gst::elementFactoryExists("x264enc") && gst::elementFactoryExists("nvv4l2decoder") && gst::elementFactoryExists("nvvideoconvert") && gst::testEncoderDecoderPipeline("x264enc ! nvv4l2decoder ! nvvideoconvert");
}

bool NVENCH265GStreamerVideoDecoder::isHardwareAccelerated() { return true; }
