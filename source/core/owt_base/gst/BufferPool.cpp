#include "BufferPool.h"

using namespace owt_base;
DEFINE_LOGGER(GStreamerBufferPool, "owt.GStreamerBufferPool");

GStreamerBufferPool::GStreamerBufferPool() = default;

bool GStreamerBufferPool::initialize(size_t bufferSize)
{
    _bufferPool = gst::unique_from_ptr(gst_buffer_pool_new());
    GstStructure* bufferPoolConfig = gst_buffer_pool_get_config(
        _bufferPool.get());
    gst_buffer_pool_config_set_params(
        bufferPoolConfig, nullptr, bufferSize, 1, 0);
    return gst_buffer_pool_set_config(_bufferPool.get(), bufferPoolConfig) && gst_buffer_pool_set_active(_bufferPool.get(), true);
}
