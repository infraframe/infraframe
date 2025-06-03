

#ifndef ENCODER_PIPELINE_H
#define ENCODER_PIPELINE_H

#include "Helpers.h"
#include "OutPtr.h"

#include <logger.h>

#include <string_view>

namespace owt_base {
enum class BitRateUnit { BitPerSec,
    KBitPerSec };

class GStreamerEncoderPipeline {
    DECLARE_LOGGER();
    std::string _encoderBitRatePropertyName;
    BitRateUnit _encoderBitRatePropertyUnit;
    std::string _encoderKeyframeIntervalPropertyName;

    gst::unique_ptr<GstPipeline> _pipeline;
    gst::unique_ptr<GstElement> _src;
    gst::unique_ptr<GstPad> _srcPad;
    gst::unique_ptr<GstElement> _converter;
    gst::unique_ptr<GstPad> _converterPad;
    gst::unique_ptr<GstElement> _encoder;
    gst::unique_ptr<GstElement> _sink;
    gst::unique_ptr<GError> _error;

public:
    GStreamerEncoderPipeline();
    ~GStreamerEncoderPipeline();

    void forceKeyFrame();
    void setBitRate(uint32_t bitRate);
    void setKeyframeInterval(int interval);
    void setResolution(uint32_t width, uint32_t height);

    GstFlowReturn pushSample(gst::unique_ptr<GstSample>& sample);
    gst::unique_ptr<GstSample> tryPullSample();

    int32_t initialize(std::string encoderBitRatePropertyName,
        BitRateUnit bitRatePropertyUnit,
        std::string keyframeIntervalPropertyName,
        std::string_view capsStr,
        std::string_view encoderPipeline);

private:
    void setEncoderProperty(const std::string& name, guint value);
};
} // namespace owt_base

#endif
