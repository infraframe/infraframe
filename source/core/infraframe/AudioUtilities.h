// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioUtilities_h
#define AudioUtilities_h

#include <webrtc/common_types.h>

#include "MediaFramePipeline.h"

namespace infraframe {

static inline int64_t currentTimeMs()
{
    timeval time;
    gettimeofday(&time, nullptr);
    return ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

FrameFormat getAudioFrameFormat(int pltype);
bool getAudioCodecInst(infraframe::FrameFormat format, webrtc::CodecInst& audioCodec);
int getAudioPltype(infraframe::FrameFormat format);
int32_t getAudioSampleRate(const infraframe::FrameFormat format);
uint32_t getAudioChannels(const infraframe::FrameFormat format);

} /* namespace infraframe */

#endif /* AudioUtilities_h */
