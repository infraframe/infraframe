#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include "BufferPool.h"
#include "ClassMacro.h"
#include "DecoderPipeline.h"
#include "Helpers.h"

// #include <I420BufferManager.h>
#include <logger.h>

#include <webrtc/api/video_codecs/video_decoder.h>
#include <webrtc/common_video/include/i420_buffer_pool.h>
#include <webrtc/media/base/codec.h>
#include <webrtc/modules/video_coding/include/video_codec_interface.h>

#include <boost/scoped_ptr.hpp>

namespace owt_base {
class GStreamerVideoDecoder : public webrtc::VideoDecoder {
    DECLARE_LOGGER();
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
    // boost::scoped_ptr<I420BufferManager> m_bufferManager;
    webrtc::I420BufferPool _bufferManager;

public:
    GStreamerVideoDecoder(std::string mediaTypeCaps,
        std::string decoderPipeline,
        bool resetPipelineOnSizeChanges = false);
    ~GStreamerVideoDecoder() override = default;

    DECLARE_NOT_COPYABLE(GStreamerVideoDecoder);
    DECLARE_NOT_MOVABLE(GStreamerVideoDecoder);

    int32_t Release() override;

    // int32_t Decode(const webrtc::EncodedImage& inputImage,
    // bool missingFrames,
    // int64_t renderTimeMs) override;
    int32_t Decode(const webrtc::EncodedImage& input_image,
        bool missing_frames,
        const webrtc::RTPFragmentationHeader* fragmentation,
        const webrtc::CodecSpecificInfo* codec_specific_info = NULL,
        int64_t render_time_ms = -1) override;

    // bool Configure(const webrtc::VideoDecoder::Settings& settings) override;
    int32_t InitDecode(const webrtc::VideoCodec* codec_settings, int32_t number_of_cores);

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
} // namespace owt_base

#endif
