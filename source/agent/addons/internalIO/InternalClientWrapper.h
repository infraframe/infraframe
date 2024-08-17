// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALCLIENTWRAPPER_H
#define INTERNALCLIENTWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <InternalClient.h>
#include <logger.h>
#include <nan.h>

/*
 * Wrapper class of infraframe::InternalClient
 */
class InternalClient : public FrameSource,
                       public infraframe::InternalClient::Listener {
public:
    DECLARE_LOGGER();
    static NAN_MODULE_INIT(Init);

    infraframe::InternalClient* me;
    boost::mutex stats_lock;
    std::queue<std::string> stats_messages;

    // Implements infraframe::InternalClient::Listener
    void onConnected() override;
    void onDisconnected() override;

private:
    InternalClient();
    ~InternalClient();

    Nan::Callback* stats_callback_;
    uv_async_t* async_stats_;

    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(New);

    static NAN_METHOD(close);

    static NAN_METHOD(addDestination);

    static NAN_METHOD(removeDestination);

    static NAUV_WORK_CB(statsCallback);
};

#endif
