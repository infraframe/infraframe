// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AVStreamOutWrap_h
#define AVStreamOutWrap_h

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "../../addons/common/NodeEventRegistry.h"
#include <AVStreamOut.h>
#include <nan.h>

/*
 * Wrapper class of infraframe::AVStreamOut
 */
class AVStreamOutWrap : public FrameDestination, public NodeEventRegistry {
public:
    static void Init(v8::Local<v8::Object>);
    static void Init(v8::Local<v8::Object>, v8::Local<v8::Object>);
    infraframe::AVStreamOut* me;

private:
    AVStreamOutWrap();
    ~AVStreamOutWrap();
    static v8::Persistent<v8::Function> constructor;

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void close(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif // AVStreamOutWrap_h
