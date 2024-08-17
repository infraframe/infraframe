#ifndef GstreamerFrameEncoder_H
#define GstreamerFrameEncoder_H

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "MediaFramePipeline.h"
#include "logger.h"

#include <cstdint>
#include <iomanip> // 添加这个头文件
#include <map>
#include <memory>

namespace infraframe {

class GstreamerFrameEncoder : public VideoFrameEncoder {
    DECLARE_LOGGER();

public:
    GstreamerFrameEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~GstreamerFrameEncoder();

    FrameFormat getInputFormat() { return FRAME_FORMAT_I420; }
    bool initialize(uint32_t width, uint32_t height, uint32_t bitrateKbps, uint32_t frameRate, uint32_t keyFrameIntervalSeconds);
    void onFrame(const Frame&);
    void setBitrate(unsigned short kbps);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame();
    void requestKeyFrame(int32_t streamId);

    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();

    int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* destination);
    void degenerateStream(int32_t streamId);

    static bool supportFormat(FrameFormat format);

private:
    void initGstreamer();
    void deinitGstreamer();
    void createPipeline();
    void configureElements(uint32_t width, uint32_t height, uint32_t bitrateKbps, uint32_t frameRate, uint32_t keyFrameIntervalSeconds);
    void startPipeline();
    void stopPipeline();

    GstBuffer* convertToInputFormat(const infraframe::Frame& frame);
    static gboolean busCallback(GstBus* bus, GstMessage* message, gpointer user_data);
    static GstFlowReturn onNewSampleFromSink(GstElement* sink, gpointer user_data);
    static void onHandoff(GstElement* identity, GstBuffer* buffer, GstPad* pad, gpointer user_data);
    static bool isEncoderSupported(const char* encoderName);
    static void start_feed(GstElement* source, guint size, GstElement* pipeline) {};
    FrameDestination* m_dest;

    FrameFormat inputFormat;
    VideoCodecProfile codecProfile;
    bool useSimulcast;
    GstElement* pipeline;
    GstElement* source;
    GstElement* encoder;
    GstElement* sink;
    GstElement* identity;
    GstElement* nvvideoconvert;
    GstElement* capsfilter;
    GstElement* h264parser;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;
    uint32_t m_keyFrameIntervalSeconds;
    bool m_forceIDR;
    bool m_setBitRateFlag;
    bool m_requestKeyFrameFlag;
    unsigned int newBitrate;

    boost::shared_mutex m_mutex;
    std::map<int32_t, GstElement*> m_streamPipelines;
    int32_t m_nextStreamId;
};

} // namespace infraframe

#endif // GstreamerFrameEncoder_H
