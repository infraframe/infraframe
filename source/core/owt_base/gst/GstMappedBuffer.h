#ifndef GST_MAPPED_BUFFER_H
#define GST_MAPPED_BUFFER_H

#include "ClassMacro.h"

#include <gst/gst.h>
#include <gst/video/video.h>

namespace owt_base {

class GstMappedBuffer {
    DECLARE_LOGGER();
    GstBuffer* _buffer;
    GstMapInfo _bufferInfo;
    bool _isValid = false;

public:
    GstMappedBuffer(GstBuffer* buffer, GstMapFlags flags);
    ~GstMappedBuffer();

    DECLARE_NOT_COPYABLE(GstMappedBuffer);
    DECLARE_NOT_MOVABLE(GstMappedBuffer);

    [[nodiscard]] uint8_t* data() const;
    [[nodiscard]] size_t size() const;

    explicit operator bool() const { return _isValid; }
};

inline GstMappedBuffer::GstMappedBuffer(GstBuffer* buffer, GstMapFlags flags)
    : _buffer(buffer)
{
    _isValid = gst_buffer_map(buffer, &_bufferInfo, flags);
}

inline GstMappedBuffer::~GstMappedBuffer()
{
    if (_isValid) {
        gst_buffer_unmap(_buffer, &_bufferInfo);
    }
    _isValid = false;
}

inline uint8_t* GstMappedBuffer::data() const
{
    return _isValid ? _bufferInfo.data : nullptr;
}

inline size_t GstMappedBuffer::size() const
{
    return _isValid ? _bufferInfo.size : 0;
}
} // namespace owt_base

#endif
