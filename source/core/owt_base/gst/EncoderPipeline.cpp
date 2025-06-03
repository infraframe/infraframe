

#include "EncoderPipeline.h"
#include "MessageHandling.h"
#include "OutPtr.h"

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <webrtc/modules/video_coding/include/video_codec_interface.h>

#include <cmath>

using namespace owt_base;
using namespace owt_base::internal;
using namespace std;

DEFINE_LOGGER(GStreamerEncoderPipeline, "owt.GStreamerEncoderPipeline");

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

void GStreamerEncoderPipeline::setResolution(uint32_t width, uint32_t height)
{
    if (!_converter || !_converterPad) {
        return;
    }

    gst::unique_ptr<GstCaps> caps = gst::unique_from_ptr(gst_pad_get_current_caps(_converterPad.get()));
    if (!caps) {
        caps = gst::unique_from_ptr(gst_pad_query_caps(_converterPad.get(), nullptr));
    }

    GValue w = G_VALUE_INIT;
    GValue h = G_VALUE_INIT;
    g_value_init(&w, G_TYPE_INT);
    g_value_init(&h, G_TYPE_INT);
    g_value_set_int(&w, width);
    g_value_set_int(&h, height);
    gst_caps_set_value(caps.get(), "width", &w);
    gst_caps_set_value(caps.get(), "height", &h);
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
        + " ! queue ! " + string(encoderPipeline) + " ! capsfilter caps="
        + string(capsStr) + " ! queue ! appsink name=sink sync=false";

    _pipeline = gst::unique_from_ptr(
        GST_PIPELINE(gst_parse_launch(pipelineStr.c_str(), out_ptr(_error))));
    if (_error) {
        ELOG_ERROR("Failed to create pipeline: %s", _error->message);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _src = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "src"));
    if (!_src) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an appsrc named src.");
        ELOG_ERROR("The pipeline(%p) must contain an appsrc named src.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _srcPad = gst::unique_from_ptr(gst_element_get_static_pad(_src.get(), "src"));
    if (!_srcPad) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "Unable to get the src pad of the appsrc");
        ELOG_ERROR("The pipeline(%p) unable to get the src pad of the appsrc.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _converter = gst::unique_from_ptr(gst_bin_get_by_name(GST_BIN(_pipeline.get()), "converter"));
    if (!_converter) {
        GST_ERROR_OBJECT(_pipeline.get(), "The pipeline must contain an converter named converter.");
        ELOG_ERROR("The pipeline(%p) must contain an converter named converter.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _converterPad = gst::unique_from_ptr(gst_element_get_static_pad(_converter.get(), "src"));
    if (!_converterPad) {
        GST_ERROR_OBJECT(_pipeline.get(), "Unable to get the converter pad of the converter");
        ELOG_ERROR("The pipeline(%p) unable to get the converter pad of the converter.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _encoder = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "encoder"));
    if (!_encoder) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an encoder named encoder.");
        ELOG_ERROR("The pipeline(%p) must contain an encoder named encoder.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _sink = gst::unique_from_ptr(
        gst_bin_get_by_name(GST_BIN(_pipeline.get()), "sink"));
    if (!_sink) {
        GST_ERROR_OBJECT(_pipeline.get(),
            "The pipeline must contain an appsink named sink.");
        ELOG_ERROR("The pipeline(%p) must contain an appsink named sink.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    connectBusMessageCallback(_pipeline);

    if (gst_element_set_state(GST_ELEMENT(_pipeline.get()), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(_pipeline.get(), "Could not set state to PLAYING.");
        ELOG_ERROR("The pipeline(%p) could not set state to PLAYING.", _pipeline.get());
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

void GStreamerEncoderPipeline::setEncoderProperty(const std::string& name,
    guint value)
{
    auto dotPosition = name.find('.');
    auto valueString = std::to_string(value);

    if (dotPosition == std::string::npos) {
        g_object_set(_encoder.get(), name.c_str(), value, nullptr);
    } else {
        std::string parentName = name.substr(0, dotPosition);
        std::string childName = name.substr(dotPosition + 1);

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
        g_free(parentValues);
    }
}
