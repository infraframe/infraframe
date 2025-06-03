#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "Helpers.h"
#include "OutPtr.h"

#include <logger.h>

namespace owt_base {
class GStreamerBufferPool {
    DECLARE_LOGGER();
    gst::unique_ptr<GstBufferPool> _bufferPool;

public:
    GStreamerBufferPool();

    bool initialize(size_t bufferSize);
    gst::unique_ptr<GstBuffer> acquireBuffer();
};

inline gst::unique_ptr<GstBuffer> GStreamerBufferPool::acquireBuffer()
{
    gst::unique_ptr<GstBuffer> buffer;
    switch (gst_buffer_pool_acquire_buffer(
        _bufferPool.get(), out_ptr(buffer), nullptr)) {
    case GST_FLOW_OK:
        return buffer;
    default:
        return nullptr;
    }
}
} // namespace owt_base

#endif
