// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GstInternalIn_h
#define GstInternalIn_h

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "RawTransport.h"

class GstInternalIn : public infraframe::FrameDestination {
    DECLARE_LOGGER();

public:
    GstInternalIn(GstAppSrc* data, int framerate);
    virtual ~GstInternalIn();

    void onFrame(const infraframe::Frame& frame);
    void setPushData(bool status);

private:
    bool m_start;
    bool m_needKeyFrame;
    bool m_dumpIn;
    size_t num_frames;
    int m_framerate;
    GstAppSrc* appsrc;
};

#endif /* GstInternalIn_h */
