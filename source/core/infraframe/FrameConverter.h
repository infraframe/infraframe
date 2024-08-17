// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FrameConverter_h
#define FrameConverter_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/api/video/i420_buffer.h>

#include <logger.h>

#include "I420BufferManager.h"

namespace infraframe {

class FrameConverter {
    DECLARE_LOGGER();

public:
    FrameConverter();
    ~FrameConverter();

    bool convert(webrtc::VideoFrameBuffer* srcBuffer, webrtc::I420Buffer* dstI420Buffer);

protected:
private:
    boost::scoped_ptr<I420BufferManager> m_bufferManager;
};

} /* namespace infraframe */

#endif /* FrameConverter_h */
