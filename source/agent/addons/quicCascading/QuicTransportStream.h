// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef QUIC_TRANSPORT_STREAM_H_
#define QUIC_TRANSPORT_STREAM_H_

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>
#include <logger.h>
#include <mutex>
#include <nan.h>
#include <queue>
#include <string>
#include <unordered_map>

#include "../../core/infraframe/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/quic_transport_stream_interface.h"

/*
 * Wrapper class of TQuicServer
 *
 * Receives media from one
 */
class QuicTransportStream : public infraframe::FrameSource, public infraframe::FrameDestination, public owt::quic::QuicTransportStreamInterface::Visitor, public NanFrameNode {
    DECLARE_LOGGER();

public:
    explicit QuicTransportStream();
    explicit QuicTransportStream(owt::quic::QuicTransportStreamInterface* stream);
    virtual ~QuicTransportStream();

    static v8::Local<v8::Object> newInstance(owt::quic::QuicTransportStreamInterface* stream);

    static NAN_MODULE_INIT(init);

    static NAN_METHOD(newInstance);
    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);
    static NAN_METHOD(addInputStream);
    static NAN_METHOD(close);
    static NAN_METHOD(send);
    static NAN_METHOD(onStreamData);
    static NAN_METHOD(getId);
    static NAN_GETTER(trackKindGetter);
    static NAN_SETTER(trackKindSetter);

    static NAUV_WORK_CB(onStreamDataCallback);

    // Overrides infraframe::FrameSource.
    void onFeedback(const infraframe::FeedbackMsg&) override;

    // Overrides infraframe::FrameDestination.
    void onFrame(const infraframe::Frame&) override;
    void onVideoSourceChanged() override;

    // Overrides NanFrameNode.
    infraframe::FrameSource* FrameSource() override { return this; }
    infraframe::FrameDestination* FrameDestination() override { return this; }

    void OnData(owt::quic::QuicTransportStreamInterface* stream, char* buf, size_t len) override;

    void sendData(const std::string& data);

    uint32_t id;

private:
    void sendFeedback(const infraframe::FeedbackMsg& msg);
    typedef struct {
        boost::shared_array<char> buffer;
        int length;
    } TransportData;

    std::unordered_map<std::string, bool> hasStream_;
    size_t m_bufferSize;
    TransportData m_receiveData;
    uint32_t m_receivedBytes;
    uv_async_t m_asyncOnData;
    bool has_data_callback_;
    std::queue<std::string> data_messages;
    Nan::Callback* data_callback_;
    Nan::AsyncResource* asyncResource_;
    boost::mutex mutex;
    owt::quic::QuicTransportStreamInterface* m_stream;
    static Nan::Persistent<v8::Function> s_constructor;
    bool m_needKeyFrame;
    std::string m_trackKind;
};

#endif // QUIC_TRANSPORT_SERVER_H_
