#ifndef VIDEO_DECODER_FACTORY_H
#define VIDEO_DECODER_FACTORY_H

#include "BufferPool.h"
#include "ClassMacro.h"
#include "DecoderPipeline.h"
#include "Helpers.h"

#include <api/video_codecs/video_decoder.h>
#include <common_video/include/video_frame_buffer_pool.h>
#include <media/base/codec.h>
#include <modules/video_coding/include/video_codec_interface.h>

namespace infraframe {
class GStreamerVideoDecoder : public webrtc::VideoDecoder {
    std::string _mediaTypeCaps;
    std::string _decoderPipeline;
    bool _resetPipelineOnSizeChanges;
    std::unique_ptr<GStreamerDecoderPipeline> _gstDecoderPipeline;
    GStreamerBufferPool _gstreamerBufferPool;

    bool _keyframeNeeded;
    GstClockTime _firstBufferPts;
    GstClockTime _firstBufferDts;
    gint _width;
    gint _height;
    gst::unique_ptr<GstCaps> _caps;

    webrtc::DecodedImageCallback* _imageReadyCb;
    webrtc::VideoFrameBufferPool _webrtcBufferPool;

public:
    GStreamerVideoDecoder(std::string mediaTypeCaps,
        std::string decoderPipeline,
        bool resetPipelineOnSizeChanges = false);
    ~GStreamerVideoDecoder() override = default;

    DECLARE_NOT_COPYABLE(GStreamerVideoDecoder);
    DECLARE_NOT_MOVABLE(GStreamerVideoDecoder);

    int32_t Release() override;

    int32_t Decode(const webrtc::EncodedImage& inputImage,
        bool missingFrames,
        int64_t renderTimeMs) override;

    bool Configure(const webrtc::VideoDecoder::Settings& settings) override;

    int32_t RegisterDecodeCompleteCallback(
        webrtc::DecodedImageCallback* callback) override;

private:
    bool initializePipeline();
    void initializeBufferTimestamps(int64_t renderTimeMs,
        uint32_t imageTimestamp);

    gst::unique_ptr<GstSample> toGstSample(const webrtc::EncodedImage& inputImage,
        int64_t renderTimeMs);

    int32_t pullSample(int64_t renderTimeMs,
        uint32_t imageTimestamp,
        webrtc::VideoRotation rotation);
    GstCaps* getCapsForFrame(const webrtc::EncodedImage& image);
};
} // namespace infraframe

#endif
