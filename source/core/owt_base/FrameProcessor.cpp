// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "FrameProcessor.h"

using namespace webrtc;

namespace owt_base {

DEFINE_LOGGER(FrameProcessor, "owt.FrameProcessor");

FrameProcessor::FrameProcessor()
    : m_lastWidth(0)
    , m_lastHeight(0)
    , m_format(FRAME_FORMAT_UNKNOWN)
    , m_outWidth(-1)
    , m_outHeight(-1)
    , m_outFrameRate(-1)
    , m_clock(NULL)
{
}

FrameProcessor::~FrameProcessor()
{
    if (m_outFrameRate > 0) {
        m_jobTimer->stop();
    }
}

bool FrameProcessor::init(FrameFormat format, const uint32_t width, const uint32_t height, const uint32_t frameRate)
{
    ELOG_DEBUG_T("format(%s), size(%dx%d), frameRate(%d)", getFormatStr(format), width, height, frameRate);

    if (format != FRAME_FORMAT_MSDK && format != FRAME_FORMAT_I420) {
        ELOG_ERROR_T("Invalid format(%d)", format);
        return false;
    }

    m_format = format;
    m_outWidth = width;
    m_outHeight = height;
    m_outFrameRate = frameRate;

    m_converter.reset(new FrameConverter());

    if (m_format == FRAME_FORMAT_I420)
        m_bufferManager.reset(new I420BufferManager(3));

    m_textDrawer.reset(new owt_base::FFmpegDrawText());

    if (m_outFrameRate != 0) {
        m_clock = Clock::GetRealTimeClock();

        m_jobTimer.reset(new JobTimer(m_outFrameRate, this));
        m_jobTimer->start();
    }

    return true;
}

bool FrameProcessor::filterFrame(const Frame& frame)
{
    if (m_lastWidth != frame.additionalInfo.video.width
        || m_lastHeight != frame.additionalInfo.video.height) {
        ELOG_DEBUG_T("Stream(%s) resolution changed, %dx%d -> %dx%d", getFormatStr(frame.format), m_lastWidth, m_lastHeight, frame.additionalInfo.video.width, frame.additionalInfo.video.height);

        m_lastWidth = frame.additionalInfo.video.width;
        m_lastHeight = frame.additionalInfo.video.height;
    }

    return false;
}

void FrameProcessor::onFrame(const Frame& frame)
{
    if (filterFrame(frame))
        return;

    ELOG_TRACE_T("onFrame, format(%s), resolution(%dx%d), timestamp(%u)", getFormatStr(frame.format), frame.additionalInfo.video.width, frame.additionalInfo.video.height, frame.timeStamp / 90);

    if (!m_outFrameRate) {
        if (frame.format == m_format
            && (m_outWidth == frame.additionalInfo.video.width || m_outWidth == 0)
            && (m_outHeight == frame.additionalInfo.video.height || m_outHeight == 0)) {
            deliverFrame(frame);
            return;
        }
    }

    uint32_t width = (m_outWidth == 0 ? frame.additionalInfo.video.width : m_outWidth);
    uint32_t height = (m_outHeight == 0 ? frame.additionalInfo.video.height : m_outHeight);

    if (m_format == FRAME_FORMAT_I420) {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = m_bufferManager->getFreeBuffer(width, height);
        if (!i420Buffer) {
            ELOG_ERROR_T("No valid i420Buffer");
            return;
        }

        if (frame.format == FRAME_FORMAT_I420) {
            VideoFrame* srcFrame = (reinterpret_cast<VideoFrame*>(frame.payload));
            if (!m_converter->convert(srcFrame->video_frame_buffer().get(), i420Buffer.get())) {
                ELOG_ERROR_T("Failed to convert frame");
                return;
            }
        }
        if (!m_outFrameRate) {
            SendFrame(i420Buffer, frame.timeStamp);
        } else {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            m_activeI420Buffer = i420Buffer;
        }

        return;
    } else {
        ELOG_ERROR_T("Invalid format, input %d(%s), output %d(%s)", frame.format, getFormatStr(frame.format), m_format, getFormatStr(m_format));

        return;
    }
    return;
}

void FrameProcessor::SendFrame(rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer, uint32_t timeStamp)
{
    owt_base::Frame outFrame;
    memset(&outFrame, 0, sizeof(outFrame));

    webrtc::VideoFrame i420Frame(i420Buffer, timeStamp, 0, webrtc::kVideoRotation_0);

    outFrame.format = FRAME_FORMAT_I420;
    outFrame.payload = reinterpret_cast<uint8_t*>(&i420Frame);
    outFrame.length = 0;
    outFrame.additionalInfo.video.width = i420Frame.width();
    outFrame.additionalInfo.video.height = i420Frame.height();
    outFrame.timeStamp = timeStamp;

    m_textDrawer->drawFrame(outFrame);

    ELOG_TRACE_T("sendI420Frame, %dx%d",
        outFrame.additionalInfo.video.width,
        outFrame.additionalInfo.video.height);

    deliverFrame(outFrame);
}

void FrameProcessor::drawText(const std::string& textSpec)
{
    m_textDrawer->setText(textSpec);
    m_textDrawer->enable(true);
}

void FrameProcessor::clearText()
{
    m_textDrawer->enable(false);
}

void FrameProcessor::onTimeout()
{
    uint32_t timeStamp = kMsToRtpTimestamp * m_clock->TimeInMilliseconds();
    ;

    if (m_format == FRAME_FORMAT_I420) {
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer;
        {
            boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            i420Buffer = m_activeI420Buffer;
        }
        if (i420Buffer)
            SendFrame(i420Buffer, timeStamp);
        return;
    }
}

} //namespace owt_base
