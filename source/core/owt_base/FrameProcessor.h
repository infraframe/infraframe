// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FrameProcessor_h
#define FrameProcessor_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <vector>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/system_wrappers/include/clock.h>

#include "MediaFramePipeline.h"
#include <JobTimer.h>

#include "I420BufferManager.h"

#include "FFmpegDrawText.h"
#include "FrameConverter.h"

namespace owt_base {

class FrameProcessor : public VideoFrameProcessor, public JobTimerListener {
    DECLARE_LOGGER();

    const uint32_t kMsToRtpTimestamp = 90;

public:
    FrameProcessor();
    ~FrameProcessor();

    void onFrame(const Frame&);
    bool init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate);

    void onTimeout();

    void drawText(const std::string& textSpec);
    void clearText();

protected:
    bool filterFrame(const Frame& frame);
    void SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp);

private:
    uint32_t m_lastWidth;
    uint32_t m_lastHeight;

    FrameFormat m_format;
    uint32_t m_outWidth;
    uint32_t m_outHeight;
    uint32_t m_outFrameRate;

    boost::scoped_ptr<I420BufferManager> m_bufferManager;
    rtc::scoped_refptr<webrtc::I420Buffer> m_activeI420Buffer;

    boost::shared_mutex m_mutex;

    const webrtc::Clock* m_clock;
    boost::scoped_ptr<FrameConverter> m_converter;
    boost::scoped_ptr<JobTimer> m_jobTimer;

    boost::shared_ptr<owt_base::FFmpegDrawText> m_textDrawer;
};

} /* namespace owt_base */

#endif /* FrameProcessor_h */
