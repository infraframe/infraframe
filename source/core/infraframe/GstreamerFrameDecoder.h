#ifndef GSTREAMERFRAMEDECODER_H
#define GSTREAMERFRAMEDECODER_H

#include "MediaFramePipeline.h" // 包含定义 Frame 结构体的头文件
#include "logger.h"
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>

namespace infraframe {

class GstreamerFrameDecoder : public VideoFrameDecoder {
    DECLARE_LOGGER();

public:
    GstreamerFrameDecoder();
    ~GstreamerFrameDecoder();

    bool init(FrameFormat format);
    void onFrame(const Frame& frame);
    static bool supportFormat(FrameFormat format);

private:
    void initPipeline();
    void cleanup();
    void deliverFrame(const Frame& decodedFrame);
    static gboolean busCallback(GstBus* bus, GstMessage* message, gpointer user_data);
    static GstFlowReturn newSample(GstAppSink* sink, gpointer user_data);
    static void onPadAdded(GstElement* src, GstPad* pad, gpointer user_data);

    GstElement* m_pipeline;
    GstElement* m_decoder;
    std::unique_ptr<GMainLoop, void (*)(GMainLoop*)> m_mainLoop;
};

} // namespace infraframe

#endif // GSTREAMERFRAMEDECODER_H
