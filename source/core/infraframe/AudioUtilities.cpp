// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <webrtc/common_types.h>

#include "MediaFramePipeline.h"
#include "rtputils.h"

using namespace webrtc;

namespace infraframe {

struct AudioCodecInsMap {
    infraframe::FrameFormat format;
    CodecInst codec;
};

static const AudioCodecInsMap codecInsDB[] = {
    { infraframe::FRAME_FORMAT_PCMU,
        { PCMU_8000_PT,
            "PCMU",
            8000,
            160,
            1,
            64000 } },
    { infraframe::FRAME_FORMAT_PCMA,
        { PCMA_8000_PT,
            "PCMA",
            8000,
            160,
            1,
            64000 } },
    { infraframe::FRAME_FORMAT_ISAC16,
        { ISAC_16000_PT,
            "ISAC",
            16000,
            480,
            1,
            32000 } },
    { infraframe::FRAME_FORMAT_ISAC32,
        { ISAC_32000_PT,
            "ISAC",
            32000,
            960,
            1,
            56000 } },
    { infraframe::FRAME_FORMAT_OPUS,
        { OPUS_48000_PT,
            "opus",
            48000,
            960,
            2,
            64000 } },
    { infraframe::FRAME_FORMAT_PCM_48000_2,
        { L16_48000_PT,
            "L16",
            48000,
            480,
            2,
            768000 } },
    { infraframe::FRAME_FORMAT_ILBC,
        { ILBC_8000_PT,
            "ILBC",
            8000,
            240,
            1,
            13300 } },
    { infraframe::FRAME_FORMAT_G722_16000_1,
        { G722_16000_1_PT,
            "G722",
            16000,
            320,
            1,
            64000 } },
    { infraframe::FRAME_FORMAT_G722_16000_2,
        { G722_16000_2_PT,
            "G722",
            16000,
            320,
            2,
            64000 } },
};

static const int numCodecIns = sizeof(codecInsDB) / sizeof(codecInsDB[0]);

bool getAudioCodecInst(infraframe::FrameFormat format, CodecInst& audioCodec)
{
    for (size_t i = 0; i < numCodecIns; i++) {
        if (codecInsDB[i].format == format) {
            audioCodec = codecInsDB[i].codec;
            return true;
        }
    }

    return false;
}

int getAudioPltype(infraframe::FrameFormat format)
{
    for (size_t i = 0; i < numCodecIns; i++) {
        if (codecInsDB[i].format == format) {
            return codecInsDB[i].codec.pltype;
        }
    }

    return INVALID_PT;
}

FrameFormat getAudioFrameFormat(int pltype)
{
    for (size_t i = 0; i < numCodecIns; i++) {
        if (codecInsDB[i].codec.pltype == pltype) {
            return codecInsDB[i].format;
        }
    }

    return FRAME_FORMAT_UNKNOWN;
}

int32_t getAudioSampleRate(const infraframe::FrameFormat format)
{
    switch (format) {
    case infraframe::FRAME_FORMAT_AAC_48000_2:
        return 48000;
    case infraframe::FRAME_FORMAT_AAC:
    case infraframe::FRAME_FORMAT_AC3:
    case infraframe::FRAME_FORMAT_NELLYMOSER:
        return 0;
    default:
        break;
    }

    for (size_t i = 0; i < numCodecIns; i++) {
        if (codecInsDB[i].format == format) {
            return codecInsDB[i].codec.plfreq;
        }
    }

    return 0;
}

uint32_t getAudioChannels(const infraframe::FrameFormat format)
{
    switch (format) {
    case infraframe::FRAME_FORMAT_AAC_48000_2:
        return 2;
    case infraframe::FRAME_FORMAT_AAC:
    case infraframe::FRAME_FORMAT_AC3:
    case infraframe::FRAME_FORMAT_NELLYMOSER:
        return 0;
    default:
        break;
    }

    for (size_t i = 0; i < numCodecIns; i++) {
        if (codecInsDB[i].format == format) {
            return codecInsDB[i].codec.channels;
        }
    }

    return 0;
}

} /* namespace infraframe */
