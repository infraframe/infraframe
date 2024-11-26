// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameAnalyzerImpl_h
#define VideoFrameAnalyzerImpl_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>

#include <FrameProcessor.h>
#include <MediaFramePipeline.h>
#include <MediaUtilities.h>
#include <VCMFrameDecoder.h>
#include <VCMFrameEncoder.h>
#include <VideoFrameAnalyzer.h>
#include <FrameAnalyzer.h>

namespace mcu {

class VideoFrameAnalyzerImpl : public VideoFrameAnalyzer, public owt_base::FrameSource, public owt_base::FrameDestination {
public:
    VideoFrameAnalyzerImpl();
    ~VideoFrameAnalyzerImpl();

    bool setInput(int input, owt_base::FrameFormat, owt_base::FrameSource*);
    void unsetInput(int input);

    bool addOutput(int output,
        owt_base::FrameFormat,
        const owt_base::VideoCodecProfile profile,
        const owt_base::VideoSize&,
        const unsigned int framerateFPS,
        const unsigned int bitrateKbps,
        const unsigned int keyFrameIntervalSeconds,
        const std::string& algorithm,
        const std::string& pluginName,
        owt_base::FrameDestination*);
    void removeOutput(int output);

    void requestKeyFrame(int output);

    void onFrame(const owt_base::Frame& frame)
    {
        deliverFrame(frame);
    }

private:
    struct Input {
        owt_base::FrameSource* source;
        boost::shared_ptr<owt_base::VideoFrameDecoder> decoder;
    };

    struct Output {
        boost::shared_ptr<owt_base::VideoFrameProcessor> processor;
        boost::shared_ptr<owt_base::VideoFrameAnalyzer> analyzer;
        boost::shared_ptr<owt_base::VideoFrameEncoder> encoder;
        int streamId;
    };

    std::map<int, Input> m_inputs;
    boost::shared_mutex m_inputMutex;

    std::map<int, Output> m_outputs;
    boost::shared_mutex m_outputMutex;
};

VideoFrameAnalyzerImpl::VideoFrameAnalyzerImpl()
{
}

VideoFrameAnalyzerImpl::~VideoFrameAnalyzerImpl()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            this->removeVideoDestination(it->second.processor.get());
            it->second.processor->removeVideoDestination(it->second.analyzer.get());
            it->second.analyzer->removeVideoDestination(it->second.encoder.get());
            it->second.encoder->degenerateStream(it->second.streamId);
        }
        m_outputs.clear();
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
        for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            it->second.source->removeVideoDestination(it->second.decoder.get());
            it->second.decoder->removeVideoDestination(this);
            m_inputs.erase(it);
        }
        m_inputs.clear();
    }
}

inline bool VideoFrameAnalyzerImpl::setInput(int input, owt_base::FrameFormat format, owt_base::FrameSource* source)
{
    assert(source);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end())
        return false;

    boost::shared_ptr<owt_base::VideoFrameDecoder> decoder;

    if (!decoder && owt_base::VCMFrameDecoder::supportFormat(format))
        decoder.reset(new owt_base::VCMFrameDecoder(format));

    if (!decoder)
        return false;

    if (decoder->init(format)) {
        decoder->addVideoDestination(this);
        source->addVideoDestination(decoder.get());
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        Input in { .source = source, .decoder = decoder };
        m_inputs[input] = in;
        return true;
    }
    return false;
}

inline void VideoFrameAnalyzerImpl::unsetInput(int input)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end()) {
        it->second.source->removeVideoDestination(it->second.decoder.get());
        it->second.decoder->removeVideoDestination(this);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_inputs.erase(it);
    }
}

inline bool VideoFrameAnalyzerImpl::addOutput(int output,
    owt_base::FrameFormat format,
    const owt_base::VideoCodecProfile profile,
    const owt_base::VideoSize& rootSize,
    const unsigned int framerateFPS,
    const unsigned int bitrateKbps,
    const unsigned int keyFrameIntervalSeconds,
    const std::string& algorithm,
    const std::string& pluginName,
    owt_base::FrameDestination* dest)
{
    boost::shared_ptr<owt_base::VideoFrameEncoder> encoder;
    boost::shared_ptr<owt_base::VideoFrameProcessor> processor;
    boost::shared_ptr<owt_base::VideoFrameAnalyzer> analyzer;
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    int32_t streamId = -1;

    if (!encoder && owt_base::VCMFrameEncoder::supportFormat(format))
        encoder.reset(new owt_base::VCMFrameEncoder(format, profile, false));

    if (!encoder)
        return false;

    streamId = encoder->generateStream(rootSize.width, rootSize.height, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
    if (streamId < 0)
        return false;

    if (!processor) {
        processor.reset(new owt_base::FrameProcessor());
    }

    if (!processor->init(encoder->getInputFormat(), rootSize.width, rootSize.height, framerateFPS))
        return false;

    this->addVideoDestination(processor.get());
    if (!analyzer) {
        analyzer.reset(new owt_base::FrameAnalyzer());
    }
    if (!analyzer->init(encoder->getInputFormat(), rootSize.width, rootSize.height, framerateFPS, pluginName))
        return false;
    processor->addVideoDestination(analyzer.get());
    analyzer->addVideoDestination(encoder.get());

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    Output out { .processor = processor, .analyzer = analyzer, .encoder = encoder, .streamId = streamId };
    m_outputs[output] = out;
    return true;
}

inline void VideoFrameAnalyzerImpl::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->degenerateStream(it->second.streamId);
        if (it->second.encoder->isIdle()) {
            this->removeVideoDestination(it->second.processor.get());
            it->second.processor->removeVideoDestination(it->second.analyzer.get());
            it->second.analyzer->removeVideoDestination(it->second.encoder.get());
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outputs.erase(output);
    }
}

inline void VideoFrameAnalyzerImpl::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end())
        it->second.encoder->requestKeyFrame(it->second.streamId);
}

}
#endif
