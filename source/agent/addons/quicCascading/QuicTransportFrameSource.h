/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_QUIC_TRANSPORT_FRAME_SOURCE_H_
#define QUIC_ADDON_QUIC_TRANSPORT_FRAME_SOURCE_H_

#include <logger.h>
#include <nan.h>

#include "../../core/infraframe/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/quic_transport_stream_interface.h"

// A QuicTransportFrameSource is a hub for multiple QuicTransport inputs to a single InternalIO output.
class QuicTransportFrameSource : public infraframe::FrameSource, public infraframe::FrameDestination, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit QuicTransportFrameSource();
    ~QuicTransportFrameSource();

    static NAN_MODULE_INIT(init);

    // Overrides infraframe::FrameSource.
    void onFeedback(const infraframe::FeedbackMsg&) override;

    // Overrides infraframe::FrameDestination.
    void onFrame(const infraframe::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    infraframe::FrameSource* FrameSource() override { return this; }
    infraframe::FrameDestination* FrameDestination() override { return this; }

private:
    // new QuicTransportFrameSource(contentSessionId, options)
    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    // addInputStream(stream, kind)
    // kind could be "data", "audio" or "video".
    static NAN_METHOD(addInputStream);

    static Nan::Persistent<v8::Function> s_constructor;

    owt::quic::QuicTransportStreamInterface* m_audioStream;
    owt::quic::QuicTransportStreamInterface* m_videoStream;
    owt::quic::QuicTransportStreamInterface* m_dataStream;
};

#endif
