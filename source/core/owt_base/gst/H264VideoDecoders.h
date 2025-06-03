

#ifndef H264_VIDEO_DECODERS_H
#define H264_VIDEO_DECODERS_H

#include "VideoDecoder.h"

#include <logger.h>

namespace owt_base {
class H264GStreamerVideoDecoder : public GStreamerVideoDecoder {
    DECLARE_LOGGER();

public:
    explicit H264GStreamerVideoDecoder(std::string decoderPipeline,
        bool resetPipelineOnSizeChanges = false);
    ~H264GStreamerVideoDecoder() override = default;

    static const char* mediaTypeCaps();
    static const char* codecName();
};

class NVH264GStreamerVideoDecoder : public H264GStreamerVideoDecoder {
public:
    NVH264GStreamerVideoDecoder();
    ~NVH264GStreamerVideoDecoder() override = default;

    // [[nodiscard]] webrtc::VideoDecoder::DecoderInfo
    // GetDecoderInfo() const override;

    static bool isSupported();
    static bool isHardwareAccelerated();
};
} // namespace owt_base

#endif
