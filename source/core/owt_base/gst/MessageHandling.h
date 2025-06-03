#ifndef MESSAGE_HANDLING_H
#define MESSAGE_HANDLING_H

#include "Helpers.h"

namespace owt_base::internal {
inline void
busMessageCallback(GstBus*, GstMessage* message, GstBin* pipeline)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        GST_ERROR_OBJECT(pipeline, "Got message: %" GST_PTR_FORMAT, message);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(pipeline)) {
            GstState oldState, newState, pending;
            gst_message_parse_state_changed(message, &oldState, &newState, &pending);

            GST_INFO_OBJECT(pipeline,
                "State changed (old: %s, new: %s, pending: %s)",
                gst_element_state_get_name(oldState),
                gst_element_state_get_name(newState),
                gst_element_state_get_name(pending));
        }
        break;
    default:
        break;
    }
}

inline void connectBusMessageCallback(gst::unique_ptr<GstPipeline>& pipeline)
{
    auto bus = gst::unique_from_ptr(
        gst_pipeline_get_bus(GST_PIPELINE(pipeline.get())));
    gst_bus_add_signal_watch_full(bus.get(), 100);
    g_signal_connect(
        bus.get(), "message", G_CALLBACK(busMessageCallback), pipeline.get());
}

inline void
disconnectBusMessageCallback(gst::unique_ptr<GstPipeline>& pipeline)
{
    auto bus = gst::unique_from_ptr(
        gst_pipeline_get_bus(GST_PIPELINE(pipeline.get())));
    g_signal_handlers_disconnect_by_func(
        bus.get(),
        reinterpret_cast<gpointer>(busMessageCallback),
        pipeline.get());
}
} // namespace owt_base::internal

#endif
