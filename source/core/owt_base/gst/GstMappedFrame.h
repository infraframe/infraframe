#ifndef GST_MAPPED_FRAME_H
#define GST_MAPPED_FRAME_H

#include "ClassMacro.h"

#include <gst/gst.h>
#include <gst/video/video.h>

#include <logger.h>

namespace owt_base {

class GstMappedFrame {
    DECLARE_LOGGER();
    GstVideoFrame _frame;
    bool _isValid = false;

public:
    GstMappedFrame(GstSample* sample, GstMapFlags flags);
    GstMappedFrame(GstBuffer* buffer, GstVideoInfo* info, GstMapFlags flags);
    ~GstMappedFrame();

    DECLARE_NOT_COPYABLE(GstMappedFrame);
    DECLARE_NOT_MOVABLE(GstMappedFrame);

    uint8_t* componentData(int comp);
    int componentStride(int stride);

    int width();
    int height();
    int format();

    explicit operator bool() const { return _isValid; }
};

inline GstMappedFrame::GstMappedFrame(GstSample* sample, GstMapFlags flags)
{
    GstVideoInfo info;

    if (!gst_video_info_from_caps(&info, gst_sample_get_caps(sample))) {
        _isValid = false;
        _frame = {};
    } else {
        _isValid = gst_video_frame_map(
            &_frame, &info, gst_sample_get_buffer(sample), flags);
    }
}

inline GstMappedFrame::GstMappedFrame(GstBuffer* buffer,
    GstVideoInfo* info,
    GstMapFlags flags)
{
    _isValid = gst_video_frame_map(&_frame, info, buffer, flags);
}

inline GstMappedFrame::~GstMappedFrame()
{
    if (_isValid) {
        gst_video_frame_unmap(&_frame);
    }
    _isValid = false;
}

inline uint8_t* GstMappedFrame::componentData(int comp)
{
    return _isValid ? GST_VIDEO_FRAME_COMP_DATA(&_frame, comp) : nullptr;
}

inline int GstMappedFrame::componentStride(int stride)
{
    return _isValid ? GST_VIDEO_FRAME_COMP_STRIDE(&_frame, stride) : -1;
}

inline int GstMappedFrame::width()
{
    return _isValid ? GST_VIDEO_FRAME_WIDTH(&_frame) : -1;
}

inline int GstMappedFrame::height()
{
    return _isValid ? GST_VIDEO_FRAME_HEIGHT(&_frame) : -1;
}

inline int GstMappedFrame::format()
{
    return _isValid ? GST_VIDEO_FRAME_FORMAT(&_frame) : GST_VIDEO_FORMAT_UNKNOWN;
}
} // namespace owt_base

#endif
