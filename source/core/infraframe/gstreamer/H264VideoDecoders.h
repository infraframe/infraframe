

#ifndef H264_VIDEO_DECODER_FACTORY_H
#define H264_VIDEO_DECODER_FACTORY_H

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

class NVENCH264GStreamerVideoDecoder : public H264GStreamerVideoDecoder {
public:
    NVENCH264GStreamerVideoDecoder();
    ~NVENCH264GStreamerVideoDecoder() override = default;

    [[nodiscard]] webrtc::VideoDecoder::DecoderInfo
    GetDecoderInfo() const override;

    static bool isSupported();
    static bool isHardwareAccelerated();
};
} // namespace infraframe

#endif
