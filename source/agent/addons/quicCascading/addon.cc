// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportClient.h"
#include "QuicTransportFrameDestination.h"
#include "QuicTransportFrameSource.h"
#include "QuicTransportServer.h"
#include "QuicTransportSession.h"
#include "QuicTransportStream.h"
#include <node.h>

using namespace v8;

NAN_MODULE_INIT(InitAll)
{
    QuicTransportStream::init(target);
    QuicTransportSession::init(target);
    QuicTransportServer::init(target);
    QuicTransportClient::init(target);
    QuicTransportFrameSource::init(target);
    QuicTransportFrameDestination::init(target);
}

NODE_MODULE(addon, InitAll)
