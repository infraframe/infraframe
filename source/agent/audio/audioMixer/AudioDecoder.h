// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AudioDecoder_h
#define AudioDecoder_h

#include "MediaFramePipeline.h"
#include <webrtc/modules/include/module_common_types.h>

namespace mcu {

class AudioDecoder : public infraframe::FrameDestination {
public:
    virtual ~AudioDecoder() { }

    virtual bool init() = 0;
    virtual bool getAudioFrame(webrtc::AudioFrame* audioFrame) = 0;

    // Implements infraframe::FrameDestination
    virtual void onFrame(const infraframe::Frame& frame) = 0;
};

} /* namespace mcu */

#endif /* AudioDecoder_h */
