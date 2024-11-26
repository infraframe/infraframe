// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoAnalyzerWrapper.h"
#include <node.h>

using namespace v8;

NODE_MODULE(addon, VideoAnalyzer::Init)
