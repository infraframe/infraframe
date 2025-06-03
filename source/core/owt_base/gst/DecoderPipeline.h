

#ifndef DECODER_PIPELINE_H
#define DECODER_PIPELINE_H

#include "Helpers.h"

#include <string_view>

#include <logger.h>

namespace owt_base {
class GStreamerDecoderPipeline {
    DECLARE_LOGGER();
    gst::unique_ptr<GstPipeline> _pipeline;
    gst::unique_ptr<GstElement> _src;
    gst::unique_ptr<GstElement> _sink;
    gst::unique_ptr<GError> _error;
    bool _ready;

public:
    GStreamerDecoderPipeline();
    ~GStreamerDecoderPipeline();

    GstFlowReturn pushSample(gst::unique_ptr<GstSample>& sample);
    void getSinkState(GstState& state, GstState& pending);
    gst::unique_ptr<GstSample> tryPullSample();

    [[nodiscard]] bool ready() const;
    void setReady(bool ready);

    int32_t initialize(std::string_view capsStr,
        std::string_view decoderPipeline);
};

inline bool GStreamerDecoderPipeline::ready() const { return _ready; }

inline void GStreamerDecoderPipeline::setReady(bool ready) { _ready = ready; }
} // namespace owt_base

#endif
