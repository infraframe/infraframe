// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameWriter_h
#define VideoFrameWriter_h

#include <string>

#include <logger.h>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include "MediaFramePipeline.h"

namespace infraframe {

class VideoFrameWriter {
    DECLARE_LOGGER();

public:
    VideoFrameWriter(const std::string& name);
    ~VideoFrameWriter();

    void write(const Frame& frame);

protected:
    FILE* getVideoFp(webrtc::VideoFrameBuffer* videoFrameBuffer);
    void write(webrtc::VideoFrameBuffer* videoFrameBuffer);

private:
    std::string m_name;

    FILE* m_fp;
    uint32_t m_index;

    int32_t m_width;
    int32_t m_height;
};

}

#endif // VideoFrameWriter_h
