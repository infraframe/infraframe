/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_FAKEVIDEOPRTPACKETIZER_H_
#define QUIC_FAKEVIDEOPRTPACKETIZER_H_

#include "../RtpPacketizerInterface.h"

// Fake RTP packetizer for testing.
class FakeVideoRtpPacketizer : public VideoRtpPacketizerInterface {

public:
    explicit FakeVideoRtpPacketizer();
    virtual ~FakeVideoRtpPacketizer();

    // Overrides infraframe::FrameDestination.
    void onFrame(const infraframe::Frame&) override {};
    void onVideoSourceChanged() override {};

    // Overrides AdapterFeedbackListener.
    void onFeedback(const infraframe::FeedbackMsg& msg) override {};
    // Overrides AdapterStatsListener.
    void onAdapterStats(const rtc_adapter::AdapterStats& stats) override {};
    // Overrides AdapterDataListener.
    void onAdapterData(char* data, int len) override {};

    RtpConfig getRtpConfig() override
    {
        return RtpConfig();
    }
};

#endif
