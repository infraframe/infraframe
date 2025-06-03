

#ifndef SUPPORT_H
#define SUPPORT_H

#include <string_view>

namespace gst {
bool elementFactoryExists(const char* name);
bool testEncoderDecoderPipeline(const std::string& encoderDecoderPipeline);
} // namespace gst

#endif
