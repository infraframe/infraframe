// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOFRAMECONSTRUCTORWRAPPER_H
#define AUDIOFRAMECONSTRUCTORWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include "MediaWrapper.h"
#include <AudioFrameConstructor.h>
#include <nan.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of infraframe::AudioFrameConstructor
 */
class AudioFrameConstructor : public MediaSink {
public:
    static NAN_MODULE_INIT(Init);
    infraframe::AudioFrameConstructor* me;
    infraframe::FrameSource* src;

private:
    AudioFrameConstructor();
    ~AudioFrameConstructor();

    static NAN_METHOD(New);

    static NAN_METHOD(close);

    static NAN_METHOD(bindTransport);
    static NAN_METHOD(unbindTransport);

    static NAN_METHOD(enable);

    static NAN_METHOD(addDestination);
    static NAN_METHOD(removeDestination);

    static NAN_METHOD(source);

    static Nan::Persistent<v8::Function> constructor;
};

class AudioFrameSource : public FrameSource {
public:
    static NAN_MODULE_INIT(Init);
    infraframe::AudioFrameConstructor* me;

private:
    AudioFrameSource() {};
    ~AudioFrameSource() {};

    static NAN_METHOD(New);

    static Nan::Persistent<v8::Function> constructor;

    friend class AudioFrameConstructor;
};

#endif
