// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameTranscoderImpl_h
#define VideoFrameTranscoderImpl_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>

#include <FrameProcessor.h>
#include <MediaFramePipeline.h>
#include <MediaUtilities.h>
#include <VCMFrameDecoder.h>
#include <VCMFrameEncoder.h>
#include <VideoFrameTranscoder.h>

namespace mcu {

class VideoFrameTranscoderImpl : public VideoFrameTranscoder, public infraframe::FrameSource, public infraframe::FrameDestination {
public:
    VideoFrameTranscoderImpl();
    ~VideoFrameTranscoderImpl();

    bool setInput(int input, infraframe::FrameFormat, infraframe::FrameSource*);
    void unsetInput(int input);

    bool addOutput(int output,
        infraframe::FrameFormat,
        const infraframe::VideoCodecProfile profile,
        const infraframe::VideoSize&,
        const unsigned int framerateFPS,
        const unsigned int bitrateKbps,
        const unsigned int keyFrameIntervalSeconds,
        infraframe::FrameDestination*);
    void removeOutput(int output);

    void requestKeyFrame(int output);
    void drawText(const std::string& textSpec);
    void clearText();

    void onFrame(const infraframe::Frame& frame)
    {
        deliverFrame(frame);
    }

private:
    struct Input {
        infraframe::FrameSource* source;
        boost::shared_ptr<infraframe::VideoFrameDecoder> decoder;
    };

    struct Output {
        boost::shared_ptr<infraframe::VideoFrameProcessor> processor;
        boost::shared_ptr<infraframe::VideoFrameEncoder> encoder;
        int streamId;
    };

    std::map<int, Input> m_inputs;
    boost::shared_mutex m_inputMutex;

    std::map<int, Output> m_outputs;
    boost::shared_mutex m_outputMutex;
};

VideoFrameTranscoderImpl::VideoFrameTranscoderImpl()
{
}

VideoFrameTranscoderImpl::~VideoFrameTranscoderImpl()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            this->removeVideoDestination(it->second.processor.get());
            it->second.processor->removeVideoDestination(it->second.encoder.get());
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

inline bool VideoFrameTranscoderImpl::setInput(int input, infraframe::FrameFormat format, infraframe::FrameSource* source)
{
    assert(source);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end())
        return false;

    boost::shared_ptr<infraframe::VideoFrameDecoder> decoder;

    if (!decoder && infraframe::VCMFrameDecoder::supportFormat(format))
        decoder.reset(new infraframe::VCMFrameDecoder(format));

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

inline void VideoFrameTranscoderImpl::unsetInput(int input)
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

inline bool VideoFrameTranscoderImpl::addOutput(int output,
    infraframe::FrameFormat format,
    const infraframe::VideoCodecProfile profile,
    const infraframe::VideoSize& rootSize,
    const unsigned int framerateFPS,
    const unsigned int bitrateKbps,
    const unsigned int keyFrameIntervalSeconds,
    infraframe::FrameDestination* dest)
{
    boost::shared_ptr<infraframe::VideoFrameEncoder> encoder;
    boost::shared_ptr<infraframe::VideoFrameProcessor> processor;
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    int32_t streamId = -1;

    if (!encoder && infraframe::VCMFrameEncoder::supportFormat(format))
        encoder.reset(new infraframe::VCMFrameEncoder(format, profile, false));

    if (!encoder)
        return false;

    streamId = encoder->generateStream(rootSize.width, rootSize.height, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
    if (streamId < 0)
        return false;

    if (!processor) {
        processor.reset(new infraframe::FrameProcessor());
    }

    if (!processor->init(encoder->getInputFormat(), rootSize.width, rootSize.height, framerateFPS))
        return false;

    this->addVideoDestination(processor.get());
    processor->addVideoDestination(encoder.get());

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    Output out { .processor = processor, .encoder = encoder, .streamId = streamId };
    m_outputs[output] = out;
    return true;
}

inline void VideoFrameTranscoderImpl::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->degenerateStream(it->second.streamId);
        if (it->second.encoder->isIdle()) {
            this->removeVideoDestination(it->second.processor.get());
            it->second.processor->removeVideoDestination(it->second.encoder.get());
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outputs.erase(output);
    }
}

inline void VideoFrameTranscoderImpl::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end())
        it->second.encoder->requestKeyFrame(it->second.streamId);
}

inline void VideoFrameTranscoderImpl::drawText(const std::string& textSpec)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it)
        it->second.processor->drawText(textSpec);
}

inline void VideoFrameTranscoderImpl::clearText()
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it)
        it->second.processor->clearText();
}

}
#endif
