#include "GstreamerFrameEncoder.h"
#include "MediaUtilities.h"

namespace infraframe {

DEFINE_LOGGER(GstreamerFrameEncoder, "owt.GstreamerFrameEncoder");

GstreamerFrameEncoder::GstreamerFrameEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast)
    : inputFormat(format)
    , codecProfile(profile)
    , useSimulcast(useSimulcast)
    , pipeline(nullptr)
    , source(nullptr)
    , encoder(nullptr)
    , sink(nullptr)
    , m_forceIDR(false)
    , m_keyFrameIntervalSeconds(0)
    , m_setBitRateFlag(false)
    , m_requestKeyFrameFlag(false)
    , newBitrate(0)
    , m_bitrateKbps(0)
    , m_width(0)
    , m_height(0)
    , m_frameRate(0)
    , m_nextStreamId(1)
    , m_dest(NULL)
{
    gst_init(nullptr, nullptr);
}

GstreamerFrameEncoder::~GstreamerFrameEncoder()
{
    deinitGstreamer();
}

bool GstreamerFrameEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return false;
}

bool GstreamerFrameEncoder::isIdle()
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);

    return (m_dest == NULL);
}

bool GstreamerFrameEncoder::supportFormat(FrameFormat format)
{
    ELOG_DEBUG("supportFormat called with format: %d", format);
    switch (format) {
    case FRAME_FORMAT_H264:
        ELOG_DEBUG("supportFormat : FRAME_FORMAT_H264: %d", format);
        return true;
    case FRAME_FORMAT_H265:
        ELOG_DEBUG("supportFormat : FRAME_FORMAT_H265: %d", format);
        return true;
    case FRAME_FORMAT_I420:
        ELOG_DEBUG("supportFormat : FRAME_FORMAT_I420: %d", format);
        return true;
    case FRAME_FORMAT_VP8:
        ELOG_DEBUG("supportFormat : FRAME_FORMAT__VP8: %d", format);
        return false; // 修改为不支持 VP8 格式
    default:
        ELOG_DEBUG("Unsupported frame format: %d", format);
        return false;
    }
}

int32_t GstreamerFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, infraframe::FrameDestination* destination)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("00000000000000generateStream called with width: %u, height: %u, frameRate: %u, bitrateKbps: %u, keyFrameIntervalSeconds: %u",
        width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

    if (width == 0 || height == 0 || frameRate == 0 || bitrateKbps == 0) {
        ELOG_DEBUG("Invalid parameters passed to generateStream: width=%u, height=%u, frameRate=%u, bitrateKbps=%u, keyFrameIntervalSeconds=%u",
            width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);
        return -1;
    }

    // 检查输入格式是否支持
    if (inputFormat != FRAME_FORMAT_H264 && inputFormat != FRAME_FORMAT_H265 && inputFormat != FRAME_FORMAT_I420) {
        ELOG_ERROR("Unsupported input frame format: %d (%s)", inputFormat, getFormatStr(inputFormat));
        return -1;
    }

    m_width = width;
    m_height = height;
    m_frameRate = frameRate;
    m_bitrateKbps = bitrateKbps;
    m_keyFrameIntervalSeconds = keyFrameIntervalSeconds;
    int32_t streamId = m_nextStreamId++;
    m_streamPipelines[streamId] = pipeline;
    m_dest = destination;

    if (!initialize(m_width, m_height, m_bitrateKbps, m_frameRate, m_keyFrameIntervalSeconds)) {
        // int level = 5;
        gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
        ELOG_DEBUG("Failed to initialize GstreamerFrameEncoder");
        // return -1;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ELOG_DEBUG("Failed to start GStreamer pipeline");
        // return -1;
    }

    ELOG_DEBUG("Stream generated with streamId: %d", streamId);

    return streamId;
}

// int32_t GstreamerFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, infraframe::FrameDestination* dest) {
//     ELOG_DEBUG("Generating stream with width: %u, height: %u, frameRate: %u, bitrateKbps: %u, keyFrameIntervalSeconds: %u", width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);

//     {
//         boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
//         m_width = width;
//         m_height = height;
//         m_frameRate = frameRate;
//         m_bitrateKbps = bitrateKbps;
//         m_keyFrameIntervalSeconds = keyFrameIntervalSeconds;

//         // 初始化管道
//         if (!pipeline) {
//             configureElements(m_width, m_height, m_bitrateKbps, m_frameRate, m_keyFrameIntervalSeconds);
//         }

//         // 启动管道
//         GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
//         if (ret == GST_STATE_CHANGE_FAILURE) {
//             ELOG_DEBUG("Failed to start GStreamer pipeline");
//             return -1;
//         }

//         int32_t streamId = ++m_nextStreamId;
//         m_streamPipelines[streamId] = pipeline;
//         ELOG_DEBUG("Stream generated with streamId: %d", streamId);
//     }

//     return streamId;
// }

void GstreamerFrameEncoder::degenerateStream(int32_t streamId)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("degenerateStream called for streamId: %d", streamId);

    auto it = m_streamPipelines.find(streamId);
    if (it != m_streamPipelines.end()) {
        if (it->second) {
            ELOG_DEBUG("Setting pipeline state to NULL");
            GstStateChangeReturn ret = gst_element_set_state(it->second, GST_STATE_NULL);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                ELOG_DEBUG("Failed to set pipeline state to NULL for streamId: %d", streamId);
            } else {
                ELOG_DEBUG("Pipeline state set to NULL for streamId: %d", streamId);
            }

            ELOG_DEBUG("Unrefing pipeline");
            gst_object_unref(it->second);
            ELOG_DEBUG("Pipeline unrefed for streamId: %d", streamId);
        } else {
            ELOG_DEBUG("Pipeline is NULL for streamId: %d", streamId);
        }
        m_streamPipelines.erase(it);
    } else {
        ELOG_DEBUG("Stream ID not found: %d", streamId);
    }

    if (pipeline) {
        ELOG_DEBUG("Setting main pipeline state to NULL");
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            ELOG_DEBUG("Failed to set main pipeline state to NULL");
        } else {
            ELOG_DEBUG("Main pipeline state set to NULL");
        }

        ELOG_DEBUG("Unrefing main pipeline");
        gst_object_unref(pipeline);
        pipeline = nullptr;
        ELOG_DEBUG("Main pipeline unrefed and set to NULL");
    }

    m_dest = NULL;
}

// bool GstreamerFrameEncoder::initialize(uint32_t width, uint32_t height, uint32_t bitrateKbps, uint32_t frameRate, uint32_t keyFrameIntervalSeconds) {
//     ELOG_DEBUG("001111111111111111initialize called with width: %u, height: %u, bitrateKbps: %u, frameRate: %u",
//                width, height, bitrateKbps, frameRate);

//     createPipeline();
//     configureElements(width, height, bitrateKbps, frameRate,keyFrameIntervalSeconds);
//     if (!pipeline) {
//         ELOG_DEBUG("Failed to create GStreamer pipeline");
//         return false;
//     }

//     // configureElements(width, height, bitrateKbps, frameRate,keyFrameIntervalSeconds);
//     startPipeline();

//     ELOG_DEBUG("GStreamer pipeline created and configured successfully");

//     return true;
// }

bool GstreamerFrameEncoder::initialize(uint32_t width, uint32_t height, uint32_t bitrateKbps, uint32_t frameRate, uint32_t keyFrameIntervalSeconds)
{
    ELOG_DEBUG("initialize called with width: %u, height: %u, bitrateKbps: %u, frameRate: %u",
        width, height, bitrateKbps, frameRate);

    // if (!createPipeline()) {
    //     ELOG_DEBUG("Failed to create GStreamer pipeline");
    //     return false;
    // }

    // if (!configureElements(width, height, bitrateKbps, frameRate,keyFrameIntervalSeconds)) {
    //     ELOG_DEBUG("Failed to configure GStreamer elements");
    //     return false;
    // }

    configureElements(width, height, bitrateKbps, frameRate, keyFrameIntervalSeconds);
    startPipeline();

    if (pipeline) {
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            ELOG_DEBUG("Failed to start GStreamer pipeline");
            return false;
        } else {
            ELOG_DEBUG("=======================start GStreamer pipeline");
        }
    }

    // else{}

    ELOG_DEBUG("GStreamer pipeline created and configured successfully");
    return true;
}

void GstreamerFrameEncoder::setBitrate(unsigned short kbps, int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    auto it = m_streamPipelines.find(streamId);
    if (it != m_streamPipelines.end()) {
        g_object_set(it->second, "bitrate", kbps, nullptr);
    } else {
        ELOG_DEBUG("Stream ID not found: %d", streamId);
    }
}

void GstreamerFrameEncoder::requestKeyFrame(int32_t streamId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("Stream ID not found: %d", streamId);
    auto it = m_streamPipelines.find(streamId);
    if (it != m_streamPipelines.end()) {
        m_forceIDR = true;
    } else {
        ELOG_DEBUG("Stream ID not found: %d", streamId);
    }
}

void GstreamerFrameEncoder::onFrame(const Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    ELOG_DEBUG("onFrame called with frame format: %d", frame.format);
    ELOG_DEBUG("Frame details: timeStamp: %u", frame.timeStamp);
    ELOG_DEBUG("Additional info - width: %u, height: %u, isKeyFrame: %d",
        frame.additionalInfo.video.width,
        frame.additionalInfo.video.height,
        frame.additionalInfo.video.isKeyFrame);

    if (m_dest == NULL) {
        return;
    }

    if (m_width == 0 || m_height == 0) {
        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;
        if (!initialize(m_width, m_height, m_bitrateKbps, m_frameRate, m_keyFrameIntervalSeconds)) {
            return;
        }
    }

    // Check if the format of the incoming frame is different from the current input format
    if (frame.format != inputFormat) {
        ELOG_DEBUG("hhhhhhhhhhhhhhFrame format changed from %d to %d, reconfiguring pipeline", inputFormat, frame.format);
        inputFormat = frame.format;
        m_width = frame.additionalInfo.video.width;
        m_height = frame.additionalInfo.video.height;

        // Reinitialize the GStreamer pipeline with the new format
        deinitGstreamer();
        if (!initialize(m_width, m_height, m_bitrateKbps, m_frameRate, m_keyFrameIntervalSeconds)) {
            return;
        }
    }

    if (frame.payload == 0) {
        ELOG_DEBUG("Invalid frame: payload is null");
        return;
    }

    uint32_t length = frame.length;
    if (length == 0) {
        ELOG_DEBUG("Invalid frame: length is zero");
        // length = static_cast<uint32_t>(std::ceil(static_cast<double>(m_bitrateKbps) * 1000 / 8 / m_frameRate));
        length = m_width * m_height * 3 / 2; // Assuming I420 format as a default size
        ELOG_DEBUG("Frame length is zero, estimated length based on bitrate: %u", length);
    } else {
        ELOG_DEBUG("Invalid frame length: %u", frame.length);
    }

    if (!supportFormat(frame.format)) {
        ELOG_DEBUG("Unsupported input frame format: %d (%s)", frame.format, getFormatStr(frame.format));
        return;
    }

    // uint32_t length = m_width * m_height * 3 / 2; // Assuming I420 format as a default size
    ELOG_DEBUG("Frame payload data: %u", frame.payload);
    // ELOG_DEBUG("before........Payload first 64 bytes: %.*s", std::min(64UL, static_cast<unsigned long>(frame.length)), frame.payload);

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, length, nullptr);
    if (!buffer) {
        ELOG_DEBUG("Failed to allocate GstBuffer");
        return;
    }

    ELOG_DEBUG("Allocated GstBuffer, size: %u", length);
    ELOG_DEBUG("================GstBuffer, payload_size: %u", sizeof(frame.payload));
    // ELOG_DEBUG("Payload first 64 bytes: %.*s", std::min(64UL, static_cast<unsigned long>(frame.length)), frame.payload);

    // gst_buffer_fill(buffer, 0, frame.payload, length);
    // ELOG_DEBUG("gst_buffer_fill  successfully!!!!!!!!!!!!");

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        ELOG_DEBUG("Mapped buffer for writing, size: %lu", map.size);
        // memcpy(map.data, frame.payload, length);
        memcpy(map.data, frame.payload, sizeof(frame.payload));
        ELOG_DEBUG("Copied payload data to buffer, first 64 bytes: %u", std::min(64UL, map.size), map.data);
        gst_buffer_unmap(buffer, &map);
    } else {
        ELOG_DEBUG("Failed to map buffer in onFrame");
        gst_buffer_unref(buffer);
        return;
    }

    if (m_forceIDR) {
        ELOG_DEBUG("activate m_forceIDR");
        GstStructure* s = gst_structure_new("GstForceKeyUnit",
            "all-headers", G_TYPE_BOOLEAN, TRUE,
            "count", G_TYPE_UINT, 0,
            NULL);
        GstEvent* event = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
        gst_element_send_event(encoder, event);
        m_forceIDR = false;
        ELOG_DEBUG("Key frame request sent");
    }

    GstFlowReturn ret;
    g_signal_emit_by_name(source, "push-buffer", buffer, &ret);
    // gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        ELOG_DEBUG("Failed to push buffer to GStreamer pipeline");
    } else {
        ELOG_DEBUG("Buffer pushed successfully to appsrc");
    }
}

void GstreamerFrameEncoder::initGstreamer()
{
    gst_init(nullptr, nullptr);
    ELOG_DEBUG("GStreamer initialized");
}

void GstreamerFrameEncoder::deinitGstreamer()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
        ELOG_DEBUG("GStreamer pipeline deinitialized");
    }
}

void GstreamerFrameEncoder::onHandoff(GstElement* identity, GstBuffer* buffer, GstPad* pad, gpointer user_data)
{
    GstreamerFrameEncoder* encoder = static_cast<GstreamerFrameEncoder*>(user_data);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        ELOG_DEBUG("=========Buffer size: %lu", map.size);
        ELOG_DEBUG("===========Buffer data: %.*s", map.size, map.data);
        gst_buffer_unmap(buffer, &map);
    } else {
        ELOG_DEBUG("============Failed to map buffer in onHandoff");
    }
}

void GstreamerFrameEncoder::createPipeline()
{
    //     ELOG_DEBUG("pipeline111111111111111111111111111111111111");
    //     pipeline = gst_pipeline_new("video-encoder-pipeline");
    //     capsfilter = gst_element_factory_make("capsfilter","capsfilter");
    //     nvvideoconvert = gst_element_factory_make("nvvideoconvert","nvvideoconvert");
    //     source = gst_element_factory_make("appsrc", "video-source");
    //     h264parser = gst_element_factory_make("h264parse","h264parse");

    //     if (inputFormat == FRAME_FORMAT_H264) {
    //         ELOG_DEBUG("this is FRAME_FORMAT_H264 hhhhhhhhhhhhhhhh ");
    //         encoder = gst_element_factory_make("nvv4l2h264enc", "video-encoder");
    //     } else if (inputFormat == FRAME_FORMAT_H265) {
    //         encoder = gst_element_factory_make("nvv4l2h265enc", "video-encoder");
    //     } else if (inputFormat == FRAME_FORMAT_I420) {
    //         ELOG_DEBUG("this is FRAME_FORMAT_I420 hhhhhhhhhhhhhhhh ");
    //         encoder = gst_element_factory_make("nvv4l2h264enc", "video-encoder"); // 使用 H264 编码器处理 I420
    //     } else {
    //         ELOG_DEBUG("Unsupported input format");
    //         return;
    //     }

    //     sink = gst_element_factory_make("appsink", "video-sink");
    //     // identity = gst_element_factory_make("identity", "identity"); // 创建identity元素
    //     ELOG_DEBUG("pipeline22222222222222222222222222222222222222222");
    //     if (!pipeline || !source || !nvvideoconvert ||!capsfilter|| !encoder || !h264parser || !sink ) {
    //         ELOG_DEBUG("Failed to create GStreamer elements");
    //         return;
    //     }

    //     gst_bin_add_many(GST_BIN(pipeline), source,  nvvideoconvert,encoder, sink, nullptr); // 将identity添加到管道中
    //     if (!gst_element_link_many(source,  nvvideoconvert,encoder,sink, nullptr)) {
    //         ELOG_DEBUG("Failed to link GStreamer elements");
    //         gst_object_unref(pipeline);
    //         pipeline = nullptr;
    //     }

    //     g_signal_connect(source, "need-data", G_CALLBACK(start_feed), this);
    //     ELOG_DEBUG("pipeline133333333333333333333333333333333333333333");
    //     // 设置 appsink 信号
    //     g_object_set(sink, "emit-signals", true, nullptr);
    //     g_signal_connect(sink, "new-sample", G_CALLBACK(onNewSampleFromSink), this);

    //     ELOG_DEBUG("pipeline44444444444444444444444444444");
    //     // 连接 identity 的 handoff 信号
    //     // g_object_set(identity, "signal-handoffs", TRUE, nullptr);
    //     // g_signal_connect(identity, "handoff", G_CALLBACK(onHandoff), this);
}
GstFlowReturn GstreamerFrameEncoder::onNewSampleFromSink(GstElement* sink, gpointer user_data)
{
    ELOG_DEBUG("onNewSampleFromSink called");
    GstreamerFrameEncoder* encoder = static_cast<GstreamerFrameEncoder*>(user_data);
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) {
        ELOG_DEBUG("Failed to pull sample from appsink");
        return GST_FLOW_ERROR;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        Frame encodedFrame;
        memset(&encodedFrame, 0, sizeof(encodedFrame));

        if (encoder->inputFormat == FRAME_FORMAT_H264) {
            encodedFrame.format = FRAME_FORMAT_H264;
        } else if (encoder->inputFormat == FRAME_FORMAT_H265) {
            encodedFrame.format = FRAME_FORMAT_H265;
        } else if (encoder->inputFormat == FRAME_FORMAT_I420) {
            encodedFrame.format = FRAME_FORMAT_I420;
        } else {
            ELOG_DEBUG("Unsupported input format for encoding");
            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }

        encodedFrame.format = FRAME_FORMAT_H264;
        encodedFrame.length = map.size;
        encodedFrame.payload = map.data; // 直接使用map.data
        encodedFrame.timeStamp = GST_BUFFER_PTS(buffer);

        // 添加附加信息
        encodedFrame.additionalInfo.video.width = encoder->m_width;
        encodedFrame.additionalInfo.video.height = encoder->m_height;
        encodedFrame.additionalInfo.video.isKeyFrame = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

        ELOG_DEBUG("Frame processed, length: %u, timeStamp: %u", encodedFrame.length, encodedFrame.timeStamp);
        ELOG_DEBUG("Frame details: width: %u, height: %u, isKeyFrame: %d",
            encodedFrame.additionalInfo.video.width,
            encodedFrame.additionalInfo.video.height,
            encodedFrame.additionalInfo.video.isKeyFrame);

        encoder->m_dest->onFrame(encodedFrame);

        // 这里不需要删除encodedFrame.payload，因为它指向的是GstBuffer的数据
        gst_buffer_unmap(buffer, &map);
    } else {
        ELOG_DEBUG("Failed to map GstBuffer");
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void GstreamerFrameEncoder::configureElements(uint32_t width, uint32_t height, uint32_t bitrateKbps, uint32_t frameRate, uint32_t keyFrameIntervalSeconds)
{
    ELOG_DEBUG("Initializing GStreamer pipeline");

    if (!pipeline) {
        pipeline = gst_pipeline_new("pipeline_name");
        source = gst_element_factory_make("appsrc", "appsrc_name");
        nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert_name");
        if (inputFormat == FRAME_FORMAT_H264) {
            ELOG_DEBUG("this is FRAME_FORMAT_H264 hhhhhhhhhhhhhhhh ");
            encoder = gst_element_factory_make("nvv4l2h264enc", "video-encoder");
        } else if (inputFormat == FRAME_FORMAT_H265) {
            encoder = gst_element_factory_make("nvv4l2h265enc", "video-encoder");
        } else if (inputFormat == FRAME_FORMAT_I420) {
            ELOG_DEBUG("this is FRAME_FORMAT_I420 hhhhhhhhhhhhhhhh ");
            encoder = gst_element_factory_make("nvv4l2h264enc", "video-encoder"); // 使用 H264 编码器处理 I420
        } else {
            ELOG_DEBUG("Unsupported input format");
            return;
        }
        h264parser = gst_element_factory_make("h264parse", "h264parser_name");
        capsfilter = gst_element_factory_make("capsfilter", "capsfilter_name");
        sink = gst_element_factory_make("appsink", "appsink_name");

        if (!pipeline || !source || !nvvideoconvert || !encoder || !h264parser || !capsfilter || !sink) {
            ELOG_DEBUG("Failed to create GStreamer elements");
            return;
        }

        g_signal_connect(source, "need-data", G_CALLBACK(start_feed), this);

        auto appsrc_caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "I420",
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "framerate", GST_TYPE_FRACTION, frameRate, 1,
            nullptr);
        g_object_set(G_OBJECT(source), "caps", appsrc_caps, nullptr);

        gst_bin_add_many(GST_BIN(pipeline),
            source,
            nvvideoconvert,
            capsfilter,
            encoder,
            h264parser,
            sink,
            nullptr);
        if (!gst_element_link_many(source, nvvideoconvert, capsfilter, encoder, h264parser, sink, nullptr)) {
            ELOG_DEBUG("Failed to link GStreamer elements");
            gst_object_unref(pipeline);
            pipeline = nullptr;
            return;
        }

        ELOG_DEBUG("GStreamer pipeline created and linked successfully");

        // Configure the encoder
        g_object_set(encoder, "bitrate", bitrateKbps, nullptr);
        g_object_set(encoder, "iframeinterval", keyFrameIntervalSeconds * frameRate, nullptr);

        // Configure appsink
        g_object_set(sink, "emit-signals", TRUE, nullptr);
        g_signal_connect(sink, "new-sample", G_CALLBACK(onNewSampleFromSink), this);
    }
    //  return gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void GstreamerFrameEncoder::startPipeline()
{
    ELOG_DEBUG("Starting GStreamer pipeline");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ELOG_DEBUG("Failed to start GStreamer pipeline");
    } else {
        ELOG_DEBUG("GStreamer pipeline started");
    }
}

void GstreamerFrameEncoder::stopPipeline()
{
    ELOG_DEBUG("Stopping GStreamer pipeline");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    ELOG_DEBUG("GStreamer pipeline stopped");
}

gboolean GstreamerFrameEncoder::busCallback(GstBus* bus, GstMessage* message, gpointer user_data)
{
    // Handle GStreamer bus messages if needed
    return TRUE;
}

} // namespace infraframe
