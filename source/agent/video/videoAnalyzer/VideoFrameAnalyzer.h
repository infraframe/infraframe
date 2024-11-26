// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameAnalyzer_h
#define VideoFrameAnalyzer_h

#include "VideoHelper.h"
#include <MediaFramePipeline.h>

namespace mcu {

class VideoFrameAnalyzer {
public:
    virtual bool setInput(int input, owt_base::FrameFormat, owt_base::FrameSource*) = 0;
    virtual void unsetInput(int input) = 0;

    virtual bool addOutput(int output,
            owt_base::FrameFormat,
            const owt_base::VideoCodecProfile profile,
            const owt_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            const std::string& algorithm,
            const std::string& pluginName,
            owt_base::FrameDestination*) = 0;
    virtual void removeOutput(int output) = 0;

    virtual void requestKeyFrame(int output) = 0;
};

}
#endif
