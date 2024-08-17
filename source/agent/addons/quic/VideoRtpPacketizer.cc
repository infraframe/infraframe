/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VideoRtpPacketizer.h"

DEFINE_LOGGER(VideoRtpPacketizer, "VideoRtpPacketizer");

VideoRtpPacketizer::VideoRtpPacketizer()
    : m_rtcAdapter(std::unique_ptr<rtc_adapter::RtcAdapter>(rtc_adapter::RtcAdapterFactory::CreateRtcAdapter()))
    , m_videoSend(nullptr)
{
    if (!m_videoSend) {
        // Create Send Video Stream
        rtc_adapter::RtcAdapter::Config sendConfig;
        sendConfig.transport_cc = false;
        sendConfig.feedback_listener = this;
        sendConfig.rtp_listener = this;
        sendConfig.stats_listener = this;
        m_videoSend = std::unique_ptr<rtc_adapter::VideoSendAdapter>(m_rtcAdapter->createVideoSender(sendConfig));
    }
}

VideoRtpPacketizer::~VideoRtpPacketizer() { }

void VideoRtpPacketizer::onFrame(const infraframe::Frame& frame)
{
    if (m_videoSend) {
        m_videoSend->onFrame(frame);
    }
}

void VideoRtpPacketizer::onVideoSourceChanged() { }

void VideoRtpPacketizer::onFeedback(const infraframe::FeedbackMsg& msg)
{
    if (msg.cmd == infraframe::RTCP_PACKET) {
        if (m_videoSend) {
            m_videoSend->onRtcpData(msg.buffer.data, msg.buffer.len);
        }
    } else {
        ELOG_WARN("Only RTCP feedbacks can be handled.")
    }
}

void VideoRtpPacketizer::onAdapterStats(const rtc_adapter::AdapterStats& stats)
{
    ELOG_DEBUG("Received adapter stats, do nothing.");
}

void VideoRtpPacketizer::onAdapterData(char* data, int len)
{
    infraframe::Frame frame;
    frame.format = infraframe::FRAME_FORMAT_RTP;
    frame.length = len;
    frame.payload = reinterpret_cast<uint8_t*>(data);
    deliverFrame(frame);
}

RtpConfig VideoRtpPacketizer::getRtpConfig()
{
    RtpConfig rtpConfig;
    if (m_videoSend) {
        rtpConfig.ssrc = m_videoSend->ssrc();
    }
    return rtpConfig;
}
