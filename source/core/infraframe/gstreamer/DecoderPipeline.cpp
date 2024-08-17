

#include "DecoderPipeline.h"
#include "MessageHandling.h"
#include "OutPtr.h"

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <modules/video_coding/include/video_codec_interface.h>

using namespace infraframe;
using namespace infraframe::internal;
using namespace std;

GStreamerDecoderPipeline::GStreamerDecoderPipeline()
    : _pipeline { nullptr }
    , _src { nullptr }
    , _sink { nullptr }
    , _ready { false }
{
}

GStreamerDecoderPipeline::~GStreamerDecoderPipeline()
{
    if (_pipeline) {
        disconnectBusMessageCallback(_pipeline);
    }
}

GstFlowReturn
GStreamerDecoderPipeline::pushSample(gst::unique_ptr<GstSample>& sample)
{
    return gst_app_src_push_sample(GST_APP_SRC(_src.get()), sample.get());
}

void GStreamerDecoderPipeline::getSinkState(GstState& state,
    GstState& pending)
{
    gst_element_get_state(_sink.get(), &state, &pending, GST_SECOND / 10);
}

gst::unique_ptr<GstSample> GStreamerDecoderPipeline::tryPullSample()
{
    return gst::unique_from_ptr(
        gst_app_sink_try_pull_sample(GST_APP_SINK(_sink.get()), GST_SECOND / 10));
}

int32_t GStreamerDecoderPipeline::initialize(string_view capsStr,
    string_view decoderPipeline)
{
    if (_pipeline) {
        disconnectBusMessageCallback(_pipeline);
    }

    string pipelineStr = string(
                             "appsrc name=src emit-signals=true is-live=true format=time caps=")
        + string(capsStr) +

        " ! queue ! " + string(decoderPipeline) +

        " ! capsfilter caps=video/x-raw,format=I420"

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
