

#include "EncoderPipeline.h"
#include "MessageHandling.h"
#include "OutPtr.h"

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <modules/video_coding/include/video_codec_interface.h>

#include <cmath>

using namespace infraframe;
using namespace infraframe::internal;
using namespace std;

GStreamerEncoderPipeline::GStreamerEncoderPipeline()
    : _encoderBitRatePropertyUnit(BitRateUnit::BitPerSec)
{
}

GStreamerEncoderPipeline::~GStreamerEncoderPipeline()
{
    if (_pipeline) {
        disconnectBusMessageCallback(_pipeline);
    }
}

void GStreamerEncoderPipeline::forceKeyFrame()
{
    gst_pad_push_event(
        _srcPad.get(),
        gst_video_event_new_downstream_force_key_unit(GST_CLOCK_TIME_NONE,
            GST_CLOCK_TIME_NONE,
            GST_CLOCK_TIME_NONE,
            FALSE,
            1));
}

void GStreamerEncoderPipeline::setBitRate(uint32_t bitRate)
{
    if (!_encoder) {
        return;
    }

    guint scaledBitRate = 0;
    switch (_encoderBitRatePropertyUnit) {
    case BitRateUnit::BitPerSec:
        scaledBitRate = static_cast<guint>(bitRate);
        break;
    case BitRateUnit::KBitPerSec:
        scaledBitRate = static_cast<guint>(
            round(static_cast<float>(bitRate) / 1000.f));
        break;
    }

    setEncoderProperty(_encoderBitRatePropertyName, scaledBitRate);
}

void GStreamerEncoderPipeline::setKeyframeInterval(int interval)
{
    if (!_encoder) {
        return;
    }

    setEncoderProperty(_encoderKeyframeIntervalPropertyName,
        static_cast<guint>(interval));
}

GstFlowReturn
GStreamerEncoderPipeline::pushSample(gst::unique_ptr<GstSample>& sample)
{
    return gst_app_src_push_sample(GST_APP_SRC(_src.get()), sample.get());
}

gst::unique_ptr<GstSample> GStreamerEncoderPipeline::tryPullSample()
{
    return gst::unique_from_ptr(
        gst_app_sink_try_pull_sample(GST_APP_SINK(_sink.get()), GST_SECOND));
}

int32_t
GStreamerEncoderPipeline::initialize(string encoderBitRatePropertyName,
    BitRateUnit encoderBitRatePropertyUnit,
    string encoderKeyframeIntervalPropertyName,
    string_view capsStr,
    string_view encoderPipeline)
{
    _encoderBitRatePropertyName = std::move(encoderBitRatePropertyName);
    _encoderBitRatePropertyUnit = encoderBitRatePropertyUnit;
    _encoderKeyframeIntervalPropertyName = std::move(
        encoderKeyframeIntervalPropertyName);

    if (_pipeline) {
        disconnectBusMessageCallback(_pipeline);
    }

    string pipelineStr = string("appsrc name=src emit-signals=true is-live=true "
                                "format=time caps=video/x-raw,format=I420")
        +

        " ! queue ! " + string(encoderPipeline) +

        " ! capsfilter caps=" + string(capsStr) +

        " ! queue"
        " ! appsink name=sink sync=false";

    GST_INFO("Pipeline: %s", pipelineStr.c_str());
    _pipeline = gst::unique_from_ptr(
        GST_PIPELINE(gst_parse_launch(pipelineStr.c_str(), out_ptr(_error))));
    if (_error) {
        GST_ERROR("Failed to create pipeline: %s", _error->message);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _src = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "src"));
    if (!_src) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an appsrc named src.");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _srcPad = gst::unique_from_ptr(gst_element_get_static_pad(_src.get(), "src"));
    if (!_srcPad) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "Unable to get the src pad of the appsrc");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _encoder = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "encoder"));
    if (!_encoder) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an encoder named encoder.");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _sink = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "sink"));
    if (!_sink) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an appsink named sink.");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    connectBusMessageCallback(_pipeline);

    if (gst_element_set_state(GST_ELEMENT(_pipeline.get()), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(_pipeline.get(), "Could not set state to PLAYING.");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

#ifdef DEBUG_GSTREAMER
    GST_DEBUG_BIN_TO_DOT_FILE(
        GST_BIN(pipeline()), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
#endif

    return WEBRTC_VIDEO_CODEC_OK;
}

void GStreamerEncoderPipeline::setEncoderProperty(const std::string& name,
    guint value)
{
    auto dotPosition = name.find('.');
    auto valueString = std::to_string(value);

    if (dotPosition == std::string::npos) {
        GST_INFO("Set encoder property - %s=%s", name.c_str(), valueString.c_str());
        g_object_set(_encoder.get(), name.c_str(), value, nullptr);
    } else {
        std::string parentName = name.substr(0, dotPosition);
        std::string childName = name.substr(dotPosition + 1);
        GST_INFO("Set encoder property - %s.%s=%s",
            parentName.c_str(),
            childName.c_str(),
            valueString.c_str());

        GstStructure* structureRaw;
        g_object_get(_encoder.get(), parentName.c_str(), &structureRaw, nullptr);

        if (!structureRaw) {
            structureRaw = gst_structure_new_empty(parentName.c_str());
        }
        auto structure = gst::unique_from_ptr(structureRaw);

        gst_structure_set(
            structure.get(), childName.c_str(), G_TYPE_UINT, value, nullptr);
        g_object_set(_encoder.get(), parentName.c_str(), structure.get(), nullptr);

        gchar* parentValues = gst_structure_to_string(structure.get());
        GST_INFO("Parent values - %s", parentValues);
        g_free(parentValues);
    }
}
