// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameTranscoder_h
#define VideoFrameTranscoder_h

#include "VideoHelper.h"
#include <MediaFramePipeline.h>

namespace mcu {

class VideoFrameTranscoder {
public:
    virtual bool setInput(int input, infraframe::FrameFormat, infraframe::FrameSource*) = 0;
    virtual void unsetInput(int input) = 0;

    virtual bool addOutput(int output,
        infraframe::FrameFormat,
        const infraframe::VideoCodecProfile profile,
        const infraframe::VideoSize&,
        const unsigned int framerateFPS,
        const unsigned int bitrateKbps,
        const unsigned int keyFrameIntervalSeconds,
        infraframe::FrameDestination*)
        = 0;
    virtual void removeOutput(int output) = 0;

    virtual void requestKeyFrame(int output) = 0;
    virtual void drawText(const std::string& textSpec) = 0;
    virtual void clearText() = 0;
};

}
#endif
