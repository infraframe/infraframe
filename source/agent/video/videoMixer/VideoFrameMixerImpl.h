// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameMixerImpl_h
#define VideoFrameMixerImpl_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>

#include <MediaFramePipeline.h>
#include <MediaUtilities.h>
#include <VCMFrameDecoder.h>
#include <VCMFrameEncoder.h>

#include "SoftVideoCompositor.h"

namespace mcu {

class CompositeIn : public infraframe::FrameDestination {
public:
    CompositeIn(int index, const std::string& avatar, boost::shared_ptr<VideoFrameCompositor> compositor)
        : m_index(index)
        , m_compositor(compositor)
    {
        m_compositor->activateInput(m_index);
        m_compositor->setAvatar(m_index, avatar);
    }

    virtual ~CompositeIn()
    {
        m_compositor->unsetAvatar(m_index);
        m_compositor->deActivateInput(m_index);
    }

    void onFrame(const infraframe::Frame& frame)
    {
        m_compositor->pushInput(m_index, frame);
    }

private:
    int m_index;
    boost::shared_ptr<VideoFrameCompositor> m_compositor;
};

class VideoFrameMixerImpl : public VideoFrameMixer {
public:
    VideoFrameMixerImpl(uint32_t maxInput, infraframe::VideoSize rootSize, infraframe::YUVColor bgColor, bool useSimulcast, bool crop);
    ~VideoFrameMixerImpl();

    bool addInput(int input, infraframe::FrameFormat, infraframe::FrameSource*, const std::string& avatar);
    void removeInput(int input);
    void setInputActive(int input, bool active);

    bool addOutput(int output,
        infraframe::FrameFormat,
        const infraframe::VideoCodecProfile profile,
        const infraframe::VideoSize&,
        const unsigned int framerateFPS,
        const unsigned int bitrateKbps,
        const unsigned int keyFrameIntervalSeconds,
        infraframe::FrameDestination*);
    void removeOutput(int output);
    void setBitrate(unsigned short kbps, int output);
    void requestKeyFrame(int output);

    void updateLayoutSolution(LayoutSolution& solution);

    void drawText(const std::string& textSpec);
    void clearText();

private:
    struct Input {
        infraframe::FrameSource* source;
        boost::shared_ptr<infraframe::VideoFrameDecoder> decoder;
        // 共享boost::shared_ptr<VideoFrameCompositor> m_compositor
        boost::shared_ptr<CompositeIn> compositorIn;
    };

    struct Output {
        boost::shared_ptr<infraframe::VideoFrameEncoder> encoder;
        int streamId;
    };

    std::map<int, Input> m_inputs;
    boost::shared_mutex m_inputMutex;

    boost::shared_ptr<VideoFrameCompositor> m_compositor;

    std::map<int, Output> m_outputs;
    boost::shared_mutex m_outputMutex;

    bool m_useSimulcast;
};

VideoFrameMixerImpl::VideoFrameMixerImpl(uint32_t maxInput, infraframe::VideoSize rootSize, infraframe::YUVColor bgColor, bool useSimulcast, bool crop)
    : m_useSimulcast(useSimulcast)
{
#ifdef ENABLE_MSDK
    if (!m_compositor)
        m_compositor.reset(new MsdkVideoCompositor(maxInput, rootSize, bgColor, crop));
#endif

    if (!m_compositor)
        m_compositor.reset(new SoftVideoCompositor(maxInput, rootSize, bgColor, crop));
}

VideoFrameMixerImpl::~VideoFrameMixerImpl()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            m_compositor->removeOutput(it->second.encoder.get());
            it->second.encoder->degenerateStream(it->second.streamId);
        }
        m_outputs.clear();
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
        for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            it->second.source->removeVideoDestination(it->second.decoder.get());
            it->second.decoder->removeVideoDestination(it->second.compositorIn.get());
            m_inputs.erase(it);
        }
        m_inputs.clear();
    }

    m_compositor.reset();
}

inline bool VideoFrameMixerImpl::addInput(int input, infraframe::FrameFormat format, infraframe::FrameSource* source, const std::string& avatar)
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
        boost::shared_ptr<CompositeIn> compositorIn(new CompositeIn(input, avatar, m_compositor));

        source->addVideoDestination(decoder.get());
        decoder->addVideoDestination(compositorIn.get());

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        Input in { .source = source, .decoder = decoder, .compositorIn = compositorIn };
        m_inputs[input] = in;
        return true;
    }

    return false;
}

inline void VideoFrameMixerImpl::removeInput(int input)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end()) {
        it->second.source->removeVideoDestination(it->second.decoder.get());
        it->second.decoder->removeVideoDestination(it->second.compositorIn.get());
        it->second.compositorIn.reset();
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_inputs.erase(it);
    }
}

inline void VideoFrameMixerImpl::setInputActive(int input, bool active)
{
    auto it = m_inputs.find(input);
    // FIXEME: Should show a black frame when input is not active
    if (it != m_inputs.end()) {
        if (active) {
            m_compositor->activateInput(input);
        } else {
            m_compositor->deActivateInput(input);
        }
    }
}

inline void VideoFrameMixerImpl::updateLayoutSolution(LayoutSolution& solution)
{
    m_compositor->updateLayoutSolution(solution);
}

inline void VideoFrameMixerImpl::setBitrate(unsigned short kbps, int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end())
        it->second.encoder->setBitrate(kbps, it->second.streamId);
}

inline void VideoFrameMixerImpl::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end())
        it->second.encoder->requestKeyFrame(it->second.streamId);
}

inline bool VideoFrameMixerImpl::addOutput(int output,
    infraframe::FrameFormat format,
    const infraframe::VideoCodecProfile profile,
    const infraframe::VideoSize& outputSize,
    const unsigned int framerateFPS,
    const unsigned int bitrateKbps,
    const unsigned int keyFrameIntervalSeconds,
    infraframe::FrameDestination* dest)
{
    boost::shared_ptr<infraframe::VideoFrameEncoder> encoder;
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);

    // find a reusable encoder.
    auto it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        if (it->second.encoder->canSimulcast(format, outputSize.width, outputSize.height))
            break;
    }

    int32_t streamId = -1;
    if (it != m_outputs.end()) { // Found a reusable encoder
        encoder = it->second.encoder;
        streamId = encoder->generateStream(outputSize.width, outputSize.height, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
        if (streamId < 0)
            return false;
    } else { // Never found a reusable encoder.
        if (!encoder && infraframe::VCMFrameEncoder::supportFormat(format))
            encoder.reset(new infraframe::VCMFrameEncoder(format, profile, m_useSimulcast));

        if (!encoder)
            return false;

        streamId = encoder->generateStream(outputSize.width, outputSize.height, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
        if (streamId < 0)
            return false;

        if (!m_compositor->addOutput(outputSize.width, outputSize.height, framerateFPS, encoder.get()))
            return false;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    Output out { .encoder = encoder, .streamId = streamId };
    m_outputs[output] = out;
    return true;
}

inline void VideoFrameMixerImpl::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->degenerateStream(it->second.streamId);
        if (it->second.encoder->isIdle()) {
            m_compositor->removeOutput(it->second.encoder.get());
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outputs.erase(output);
    }
}

inline void VideoFrameMixerImpl::drawText(const std::string& textSpec)
{
    m_compositor->drawText(textSpec);
}

inline void VideoFrameMixerImpl::clearText()
{
    m_compositor->clearText();
}

}
#endif
