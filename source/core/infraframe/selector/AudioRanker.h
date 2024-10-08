// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OWT_BASE_SELECTOR_AUDIO_RANKER_H
#define OWT_BASE_SELECTOR_AUDIO_RANKER_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "IOService.h"
#include "MediaFramePipeline.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <logger.h>

namespace infraframe {

class AudioRanker {
    DECLARE_LOGGER();

public:
    class Visitor {
    public:
        // Updates contain a vector of (streamId, ownerId) pairs
        virtual void onRankChange(
            std::vector<std::pair<std::string, std::string>> updates)
            = 0;
    };

    class AudioLevelProcessor : public FrameDestination,
                                public FrameSource {
    public:
        AudioLevelProcessor(AudioRanker* parent, FrameSource* source,
            std::string streamId, std::string ownerId);
        ~AudioLevelProcessor();

        // Implements FrameDestination
        void onFrame(const Frame&) override;
        // Implements FrameSource
        void onFeedback(const FeedbackMsg&) override;

        std::string streamId() { return m_streamId; }
        std::string ownerId() { return m_ownerId; }
        uint64_t lastUpdateTime() { return m_lastUpdateTime; }

        void setLinkedOutput(FrameDestination* output);
        FrameDestination* linkedOutput();

        void deliverOwnerData();

        void setIter(std::multimap<int, std::shared_ptr<AudioLevelProcessor>>::iterator it)
        {
            m_iter = it;
        }
        std::multimap<int, std::shared_ptr<AudioLevelProcessor>>::iterator iter() { return m_iter; }

    private:
        AudioRanker* m_parent;
        FrameSource* m_source;
        std::string m_streamId;
        std::string m_ownerId;
        uint64_t m_lastUpdateTime;
        std::multimap<int, std::shared_ptr<AudioLevelProcessor>>::iterator m_iter;
        boost::mutex m_mutex;
        FrameDestination* m_linkedOutput;
    };

    AudioRanker(Visitor* visitor, bool detectMute = true, uint32_t minChangeInterval = 200);
    virtual ~AudioRanker();

    // Append output to the top K
    void addOutput(FrameDestination* output);
    // Add input with associated stream and owner ID
    void addInput(FrameSource* input, std::string streamId, std::string ownerId);
    // Remove input with stream ID
    void removeInput(std::string streamId);
    // Update input with stream ID
    void updateInput(std::string streamId, int level);

private:
    void updateInputInternal(std::string streamId, int level, bool triggerChange = true);
    void triggerRankChange();

    bool m_detectMute;
    bool m_stashChange;
    uint32_t m_minChangeInterval;
    uint64_t m_lastChangeTime;
    std::vector<FrameDestination*> m_unlinkedOutputs;
    std::unordered_map<FrameDestination*, int> m_outputIndexes;
    std::unordered_map<std::string, std::shared_ptr<AudioLevelProcessor>> m_processors;
    // Multimap for AudioLevel : AudioLevelProcessor
    std::multimap<int, std::shared_ptr<AudioLevelProcessor>> m_topK;
    std::multimap<int, std::shared_ptr<AudioLevelProcessor>> m_others;

    std::shared_ptr<IOService> m_service;

    std::vector<std::pair<std::string, std::string>> m_lastUpdates;
    Visitor* m_visitor;
};

} // namespace infraframe

#endif // OWT_BASE_SELECTOR_AUDIO_RANKER_H
