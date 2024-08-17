/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_PRTPACKETIZERINTERFACE_H_
#define QUIC_PRTPACKETIZERINTERFACE_H_

#include "../../core/infraframe/MediaFramePipeline.h"
#include "../../core/rtc_adapter/RtcAdapter.h"
#include "../common/MediaFramePipelineWrapper.h"

// Configuration for a RTP session.
struct RtpConfig {
    // Local SSRC.
    uint32_t ssrc = 0;
    // Payload type of media.
    int payload_type = -1;
};

class VideoRtpPacketizerInterface : public infraframe::FrameSource, public infraframe::FrameDestination, public rtc_adapter::AdapterFeedbackListener, public rtc_adapter::AdapterStatsListener, public rtc_adapter::AdapterDataListener {

public:
    explicit VideoRtpPacketizerInterface() = default;
    virtual ~VideoRtpPacketizerInterface() = default;

    // Overrides infraframe::FrameDestination.
    void onFrame(const infraframe::Frame&) override = 0;
    void onVideoSourceChanged() override = 0;

    // Overrides AdapterFeedbackListener.
    void onFeedback(const infraframe::FeedbackMsg& msg) override = 0;
    // Overrides AdapterStatsListener.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override = 0;
    // Overrides AdapterDataListener.
    void onAdapterData(char* data, int len) override = 0;

    // Returns RTP configuration.
    virtual RtpConfig getRtpConfig() = 0;
};

#endif
