

#ifndef H265_VIDEO_DECODER_FACTORY_H
#define H265_VIDEO_DECODER_FACTORY_H

#include "VideoDecoder.h"

namespace infraframe {
class H264GStreamerVideoDecoder : public GStreamerVideoDecoder {
public:
    explicit H264GStreamerVideoDecoder(std::string decoderPipeline,
        bool resetPipelineOnSizeChanges = false);
    ~H264GStreamerVideoDecoder() override = default;

    static const char* mediaTypeCaps();
    static const char* codecName();
};

class NVENCH265GStreamerVideoDecoder : public H264GStreamerVideoDecoder {
public:
    NVENCH265GStreamerVideoDecoder();
    ~NVENCH265GStreamerVideoDecoder() override = default;

    [[nodiscard]] webrtc::VideoDecoder::DecoderInfo
    GetDecoderInfo() const override;

    static bool isSupported();
    static bool isHardwareAccelerated();
};

} // namespace infraframe

#endif
