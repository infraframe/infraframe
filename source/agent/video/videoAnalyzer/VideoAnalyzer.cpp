// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <webrtc/base/logging.h>
#include <webrtc/system_wrappers/include/trace.h>

#include "VideoAnalyzer.h"
#include "VideoFrameAnalyzerImpl.h"

using namespace webrtc;
using namespace owt_base;

namespace mcu {

DEFINE_LOGGER(VideoAnalyzer, "mcu.media.VideoAnalyzer");

VideoAnalyzer::VideoAnalyzer(const VideoAnalyzerConfig& config)
    : m_inputCount(0)
    , m_maxInputCount(1)
    , m_nextOutputIndex(0)
{
    if (ELOG_IS_TRACE_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
        rtc::LogMessage::LogTimestamps(true);

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    } else if (ELOG_IS_DEBUG_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_INFO);
        rtc::LogMessage::LogTimestamps(true);

        const int kTraceFilter = webrtc::kTraceNone | webrtc::kTraceTerseInfo |
            webrtc::kTraceWarning | webrtc::kTraceError |
            webrtc::kTraceCritical | webrtc::kTraceDebug |
            webrtc::kTraceInfo;

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(kTraceFilter);
    }

    m_freeInputIndexes.reserve(m_maxInputCount);
    for (size_t i = 0; i < m_maxInputCount; ++i)
        m_freeInputIndexes.push_back(true);

#ifdef ENABLE_MSDK
    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase != NULL) {
        msdkBase->setConfigHevcEncoderGaccPlugin(config.useGacc);
        msdkBase->setConfigMFETimeout(config.MFE_timeout);
    }
#endif

    ELOG_INFO("Init");

    m_frameAnalyzer.reset(new VideoFrameAnalyzerImpl());
}

VideoAnalyzer::~VideoAnalyzer()
{
    closeAll();
}

int VideoAnalyzer::useAFreeInputIndex()
{
    for (size_t i = 0; i < m_freeInputIndexes.size(); ++i) {
        if (m_freeInputIndexes[i]) {
            m_freeInputIndexes[i] = false;
            return i;
        }
    }

    return -1;
}

bool VideoAnalyzer::setInput(const std::string& inStreamID, const std::string& codec, owt_base::FrameSource* source)
{
    if (m_inputCount == m_maxInputCount) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", m_maxInputCount);
        return false;
    }

    owt_base::FrameFormat format = getFormat(codec);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it == m_inputs.end() || !it->second) {
        int index = useAFreeInputIndex();
        if (m_frameAnalyzer->setInput(index, format, source)) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_inputs[inStreamID] = index;
        }
        ++m_inputCount;
        return true;
    }

    assert("new source added with InputProcessor still available");    // should not go there
    return false;
}

void VideoAnalyzer::unsetInput(const std::string& inStreamID)
{
    int index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
        m_inputs.erase(it);
    }
    lock.unlock();

    if (index >= 0) {
        m_frameAnalyzer->unsetInput(index);
        m_freeInputIndexes[index] = true;
        --m_inputCount;
    }
}

bool VideoAnalyzer::addOutput(
    const std::string& outStreamID
    , const std::string& codec
    , const owt_base::VideoCodecProfile profile
    , const std::string& resolution
    , const unsigned int framerateFPS
    , const unsigned int bitrateKbps
    , const unsigned int keyFrameIntervalSeconds
    , const std::string& algorithm
    , const std::string& pluginName
    , owt_base::FrameDestination* dest)
{
    owt_base::FrameFormat format = getFormat(codec);
    VideoSize vSize{0, 0};
    VideoResolutionHelper::getVideoSize(resolution, vSize);
    if (m_frameAnalyzer->addOutput(m_nextOutputIndex, format, profile, vSize, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, algorithm, pluginName, dest)) {
        boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
        m_outputs[outStreamID] = m_nextOutputIndex++;
        return true;
    }
    return false;
}

void VideoAnalyzer::removeOutput(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
        m_outputs.erase(it);
    }
    lock.unlock();

    if (index != -1) {
        m_frameAnalyzer->removeOutput(index);
    }
}

void VideoAnalyzer::forceKeyFrame(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::shared_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index != -1) {
        m_frameAnalyzer->requestKeyFrame(index);
    }
}

void VideoAnalyzer::closeAll()
{
    ELOG_DEBUG("CloseAll");

    ELOG_DEBUG("Closed all media in this Analyzer");
}

}/* namespace mcu */
