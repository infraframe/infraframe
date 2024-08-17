/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_WEB_TRANSPORT_FRAME_SOURCE_H_
#define QUIC_ADDON_WEB_TRANSPORT_FRAME_SOURCE_H_

#include "../../core/infraframe/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <logger.h>
#include <nan.h>

// A WebTransportFrameSource is a hub for multiple WebTransport inputs to a single InternalIO output.
class WebTransportFrameSource : public infraframe::FrameSource, public infraframe::FrameDestination, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit WebTransportFrameSource();
    ~WebTransportFrameSource();

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
    // new WebTransportFrameSource(contentSessionId, options)
    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    // addStreamInput(stream, kind)
    // kind could be "data", "audio" or "video".
    static NAN_METHOD(addStreamInput);
    // receiver() is required by connection.js.
    static NAN_METHOD(receiver);

    static Nan::Persistent<v8::Function> s_constructor;

    owt::quic::WebTransportStreamInterface* m_audioStream;
    owt::quic::WebTransportStreamInterface* m_videoStream;
    owt::quic::WebTransportStreamInterface* m_dataStream;
};

#endif
