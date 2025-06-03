

#include "H264VideoDecoders.h"
#include "Support.h"

using namespace owt_base;
using namespace std;

DEFINE_LOGGER(H264GStreamerVideoDecoder, "owt.H264GStreamerVideoDecoder");

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

bool NVH264GStreamerVideoDecoder::isSupported()
{
    return gst::elementFactoryExists("nvv4l2decoder") && gst::elementFactoryExists("nvvideoconvert") && gst::testEncoderDecoderPipeline("nvv4l2decoder ! nvvideoconvert");
}

bool NVH264GStreamerVideoDecoder::isHardwareAccelerated() { return true; }
