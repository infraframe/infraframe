// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalConfig.h"
#include "InternalIOWrapper.h"
#include "InternalInWrapper.h"
#include "InternalOutWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports)
{
    InternalIn::Init(exports);
    InternalOut::Init(exports);
    SctpIn::Init(exports);
    SctpOut::Init(exports);
    InitInternalConfig(exports);
}

NODE_MODULE(addon, InitAll)
