// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef InternalOut_h
#define InternalOut_h

#include "MediaFramePipeline.h"
#include "RawTransport.h"

namespace infraframe {

class InternalOut : public FrameDestination, public RawTransportListener {
public:
    InternalOut(const std::string& protocol, const std::string& dest_ip, unsigned int dest_port);
    InternalOut(const std::string& protocol, const std::string& ticket,
        const std::string& dest_ip, unsigned int dest_port);
    virtual ~InternalOut();

    void onFrame(const Frame&);
    void onMetaData(const MetaData&);

    void onTransportData(char*, int len);
    void onTransportError() { }
    void onTransportConnected() { }

private:
    boost::shared_ptr<infraframe::RawTransportInterface> m_transport;
};

} /* namespace infraframe */

#endif /* InternalOut_h */
