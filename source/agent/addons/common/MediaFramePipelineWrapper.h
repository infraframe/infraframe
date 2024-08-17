// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIAFRAMEPIPELINEWRAPPER_H
#define MEDIAFRAMEPIPELINEWRAPPER_H

#include <MediaFramePipeline.h>
#include <nan.h>
#include <node.h>
#include <node_object_wrap.h>

/*
 * Wrapper class of infraframe::FrameDestination
 */
class FrameDestination : public node::ObjectWrap {
public:
    infraframe::FrameDestination* dest;
};

/*
 * Wrapper class of infraframe::FrameSource
 */
class FrameSource : public node::ObjectWrap {
public:
    infraframe::FrameSource* src;
};

/*
 * Nan::ObjectWrap of infraframe::FrameSource and infraframe::FrameDestination, represents a node in the media or data pipeline.
 */
class NanFrameNode : public Nan::ObjectWrap {
public:
    virtual infraframe::FrameSource* FrameSource() = 0;
    virtual infraframe::FrameDestination* FrameDestination() = 0;
};

#endif
