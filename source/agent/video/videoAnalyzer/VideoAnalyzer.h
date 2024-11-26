// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoAnalyzer_h
#define VideoAnalyzer_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "VideoFrameAnalyzer.h"

namespace mcu {

struct VideoAnalyzerConfig {
    bool useGacc;
    uint32_t MFE_timeout;
};

class VideoAnalyzer {
    DECLARE_LOGGER();

public:
    VideoAnalyzer(const VideoAnalyzerConfig& config);
    virtual ~VideoAnalyzer();

    bool setInput(const std::string& inStreamID, const std::string& codec, owt_base::FrameSource* source);
    void unsetInput(const std::string& inStreamID);

    bool addOutput(const std::string& outStreamID
            , const std::string& codec
            , const owt_base::VideoCodecProfile profile
            , const std::string& resolution
            , const unsigned int framerateFPS
            , const unsigned int bitrateKbps
            , const unsigned int keyFrameIntervalSeconds
            , const std::string& algorithm
            , const std::string& pluginName
            , owt_base::FrameDestination* dest);
    void removeOutput(const std::string& outStreamID);
    void forceKeyFrame(const std::string& outStreamID);

protected:
    int useAFreeInputIndex();
    void closeAll();

private:
    uint32_t m_inputCount;
    uint32_t m_maxInputCount;
    uint32_t m_nextOutputIndex;

    boost::shared_mutex m_inputsMutex;
    std::map<std::string, int> m_inputs;
    std::vector<bool> m_freeInputIndexes;

    boost::shared_mutex m_outputsMutex;
    std::map<std::string, int32_t> m_outputs;

    boost::shared_ptr<VideoFrameAnalyzer> m_frameAnalyzer;
};

} /* namespace mcu */
#endif /* VideoAnalyzer_h */
