// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioFramePacketizer.h"
#include "AudioUtilitiesNew.h"

using namespace rtc_adapter;

namespace infraframe {

DEFINE_LOGGER(AudioFramePacketizer, "infraframe.AudioFramePacketizer");

AudioFramePacketizer::AudioFramePacketizer(AudioFramePacketizer::Config& config)
    : m_enabled(true)
    , m_frameFormat(FRAME_FORMAT_UNKNOWN)
    , m_lastOriginSeqNo(0)
    , m_seqNo(0)
    , m_ssrc(0)
    , m_rtcAdapter(RtcAdapterFactory::CreateRtcAdapter())
    , m_audioSend(nullptr)
    , m_firstFrame(false)
{
    audio_sink_ = nullptr;
    init(config);
}

AudioFramePacketizer::~AudioFramePacketizer()
{
    close();
    if (m_audioSend) {
        m_rtcAdapter->destoryAudioSender(m_audioSend);
        m_rtcAdapter.reset();
        m_audioSend = nullptr;
    }
}

void AudioFramePacketizer::bindTransport(erizo::MediaSink* sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    audio_sink_ = sink;
    audio_sink_->setAudioSinkSSRC(m_audioSend->ssrc());
    erizo::FeedbackSource* fbSource = audio_sink_->getFeedbackSource();
    if (fbSource)
        fbSource->setFeedbackSink(this);
}

void AudioFramePacketizer::unbindTransport()
{
    boost::unique_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (audio_sink_) {
        audio_sink_ = nullptr;
    }
}

void AudioFramePacketizer::setOwner(std::string owner)
{
    m_firstFrame = false;
    m_owner = owner;
}

int AudioFramePacketizer::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet)
{
    if (m_audioSend) {
        m_audioSend->onRtcpData(data_packet->data, data_packet->length);
        return data_packet->length;
    }
    return 0;
}

void AudioFramePacketizer::receiveRtpData(char* buf, int len, erizoExtra::DataType type, uint32_t channelId)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (!audio_sink_) {
        return;
    }

    assert(type == erizoExtra::AUDIO);
    audio_sink_->deliverAudioData(std::make_shared<erizo::DataPacket>(0, buf, len, erizo::AUDIO_PACKET));
}

void AudioFramePacketizer::onFrame(const Frame& frame)
{
    if (!m_enabled) {
        return;
    }

    if (!m_firstFrame) {
        m_firstFrame = true;
        FeedbackMsg feedback = { .type = AUDIO_FEEDBACK, .cmd = REQUEST_OWNER_ID };
        deliverFeedbackMsg(feedback);
    }

    boost::shared_lock<boost::shared_mutex> lock1(m_transport_mutex);
    if (!audio_sink_) {
        return;
    }
    lock1.unlock();

    if (frame.length <= 0)
        return;

    if (!m_sourceOwner.empty() && m_owner == m_sourceOwner)
        return;

    if (frame.format != m_frameFormat) {
        m_frameFormat = frame.format;
    }

    if (m_audioSend) {
        m_audioSend->onFrame(frame);
    }
}

void AudioFramePacketizer::onMetaData(const MetaData& metadata)
{
    if (!m_enabled) {
        return;
    }

    if (metadata.type == META_DATA_OWNER_ID) {
        m_sourceOwner = std::string((char*)metadata.payload, metadata.length);
    }
}

bool AudioFramePacketizer::init(AudioFramePacketizer::Config& config)
{
    if (!m_audioSend) {
        // Create Send audio Stream
        rtc_adapter::RtcAdapter::Config sendConfig;
        sendConfig.rtp_listener = this;
        sendConfig.stats_listener = this;
        if (!config.mid.empty()) {
            memset(sendConfig.mid, 0, sizeof(sendConfig.mid));
            strncat(sendConfig.mid, config.mid.c_str(), sizeof(sendConfig.mid) - 1);
            sendConfig.mid_ext = config.midExtId;
        }
        m_audioSend = m_rtcAdapter->createAudioSender(sendConfig);
        m_ssrc = m_audioSend->ssrc();
        return true;
    }
    return false;
}

void AudioFramePacketizer::onAdapterStats(const AdapterStats& stats) { }

void AudioFramePacketizer::onAdapterData(char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_transport_mutex);
    if (audio_sink_) {
        audio_sink_->deliverAudioData(std::make_shared<erizo::DataPacket>(0, data, len, erizo::AUDIO_PACKET));
    }
}

void AudioFramePacketizer::close()
{
    unbindTransport();
}

int AudioFramePacketizer::sendPLI()
{
    return 0;
}
}
