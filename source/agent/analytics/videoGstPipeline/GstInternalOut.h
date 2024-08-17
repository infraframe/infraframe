// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef GstInternalOut_h
#define GstInternalOut_h

#include <gst/gst.h>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "RawTransport.h"

class GstInternalOut : public infraframe::FrameSource, public infraframe::FrameDestination {
    DECLARE_LOGGER();

public:
    GstInternalOut();
    virtual ~GstInternalOut();

    void setPad(GstPad* pad);

    // Implements FrameSource
    void onFeedback(const infraframe::FeedbackMsg&);

    void onFrame(const infraframe::Frame& frame);

private:
    GstPad* encoder_pad;
};

#endif /* GstInternalOut_h */
