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

namespace owt_base {

class FrameConverter {
    DECLARE_LOGGER();

public:
    FrameConverter(bool useMsdkVpp = true);
    ~FrameConverter();

    bool convert(webrtc::VideoFrameBuffer *srcBuffer, webrtc::I420Buffer *dstI420Buffer);

protected:

private:
    boost::scoped_ptr<I420BufferManager> m_bufferManager;
};

} /* namespace owt_base */

#endif /* FrameConverter_h */

