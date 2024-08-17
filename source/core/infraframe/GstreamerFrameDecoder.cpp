#include "GstreamerFrameDecoder.h"

using namespace infraframe;

DEFINE_LOGGER(GstreamerFrameDecoder, "owt.GstreamerFrameDecoder");

GstreamerFrameDecoder::GstreamerFrameDecoder()
    : m_pipeline(nullptr)
    , m_decoder(nullptr)
    , m_mainLoop(g_main_loop_new(nullptr, FALSE), g_main_loop_unref)
{
    initPipeline();
}

GstreamerFrameDecoder::~GstreamerFrameDecoder()
{
    cleanup();
}

bool GstreamerFrameDecoder::supportFormat(FrameFormat format)
{
    // 实现支持的格式判断逻辑
    // 假设支持 I420 格式
    return format == FRAME_FORMAT_I420;
}

void GstreamerFrameDecoder::initPipeline()
{
    gst_init(nullptr, nullptr);

    m_pipeline = gst_pipeline_new("video-pipeline");
    m_decoder = gst_element_factory_make("decodebin", "video-decoder");

    if (!m_pipeline || !m_decoder) {
        ELOG_DEBUG("Failed to create GStreamer elements");
        cleanup();
        return;
    }

    gst_bin_add_many(GST_BIN(m_pipeline), m_decoder, nullptr);

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_watch(bus, busCallback, this);
    gst_object_unref(bus);

    g_signal_connect(m_decoder, "pad-added", G_CALLBACK(onPadAdded), this);
}

void GstreamerFrameDecoder::cleanup()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(m_pipeline));
        m_pipeline = nullptr;
    }

    if (m_mainLoop) {
        g_main_loop_quit(m_mainLoop.get());
    }

    gst_deinit();
}

bool GstreamerFrameDecoder::init(FrameFormat format)
{
    const char* decoderElementName = nullptr;
    switch (format) {
    case FRAME_FORMAT_H264:
        decoderElementName = "nvv4l2decoder";
        break;
    case FRAME_FORMAT_H265:
        decoderElementName = "nvv4l2decoder";
        break;
    case FRAME_FORMAT_VP8:
        decoderElementName = "nvvp8decoder";
        break;
    default:
        ELOG_DEBUG("Unsupported video frame format %s(%d)", getFormatStr(format), format);
        return false;
    }

    m_decoder = gst_element_factory_make(decoderElementName, "decoder");
    if (!m_decoder) {
        ELOG_DEBUG("Failed to create decoder");
        return false;
    }

    gst_bin_add(GST_BIN(m_pipeline), m_decoder);

    GstElement* sink = gst_element_factory_make("appsink", "video-sink");
    if (!sink) {
        ELOG_DEBUG("Failed to create appsink");
        cleanup();
        return false;
    }

    gst_bin_add(GST_BIN(m_pipeline), sink);
    gst_element_link(m_decoder, sink);

    g_signal_connect(sink, "new-sample", G_CALLBACK(newSample), this);

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

    return true;
}

void GstreamerFrameDecoder::onFrame(const Frame& frame)
{
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame.length, nullptr);
    gst_buffer_fill(buffer, 0, frame.payload, frame.length);

    GstAppSrc* src = GST_APP_SRC(m_decoder);
    gst_app_src_push_buffer(src, buffer);
}

void GstreamerFrameDecoder::deliverFrame(const Frame& decodedFrame)
{
    // 处理解码后的帧数据，例如发送到其他组件或进一步处理
}

gboolean GstreamerFrameDecoder::busCallback(GstBus* bus, GstMessage* message, gpointer user_data)
{
    GstreamerFrameDecoder* decoder = static_cast<GstreamerFrameDecoder*>(user_data);
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;
        gst_message_parse_error(message, &error, &debug);
        ELOG_DEBUG("GStreamer error: %s", error->message);
        g_error_free(error);
        g_free(debug);
        decoder->cleanup();
        break;
    }
    case GST_MESSAGE_EOS:
        ELOG_INFO("End of stream");
        decoder->cleanup();
        break;
    default:
        break;
    }
    return TRUE;
}

GstFlowReturn GstreamerFrameDecoder::newSample(GstAppSink* sink, gpointer user_data)
{
    GstreamerFrameDecoder* decoder = static_cast<GstreamerFrameDecoder*>(user_data);
    GstSample* sample = gst_app_sink_pull_sample(sink);

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    Frame decodedFrame;
    decodedFrame.format = FRAME_FORMAT_I420; // 假设解码后的格式为 I420
    decodedFrame.payload = new uint8_t[map.size];
    std::memcpy(decodedFrame.payload, map.data, map.size);
    decodedFrame.length = map.size;
    decodedFrame.timeStamp = 0; // 你可以根据需要设置时间戳

    // 假设从缓冲区中提取视频宽度和高度
    GstCaps* caps = gst_sample_get_caps(sample);
    GstStructure* structure = gst_caps_get_structure(caps, 0);

    gint width, height;
    if (gst_structure_get_int(structure, "width", &width)) {
        decodedFrame.additionalInfo.video.width = static_cast<uint16_t>(width);
    }
    if (gst_structure_get_int(structure, "height", &height)) {
        decodedFrame.additionalInfo.video.height = static_cast<uint16_t>(height);
    }

    decoder->deliverFrame(decodedFrame);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void GstreamerFrameDecoder::onPadAdded(GstElement* src, GstPad* pad, gpointer user_data)
{
    GstreamerFrameDecoder* decoder = static_cast<GstreamerFrameDecoder*>(user_data);
    GstPad* sinkPad = gst_element_get_static_pad(decoder->m_decoder, "sink");
    gst_pad_link(pad, sinkPad);
    gst_object_unref(sinkPad);
}
