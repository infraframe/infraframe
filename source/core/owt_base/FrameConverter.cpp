// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "FrameConverter.h"

#include <libyuv/convert.h>
#include <libyuv/planar_functions.h>
#include <libyuv/scale.h>

namespace owt_base {

DEFINE_LOGGER(FrameConverter, "owt.FrameConverter");

FrameConverter::FrameConverter(bool useMsdkVpp)
{
    m_bufferManager.reset(new I420BufferManager(2));
}

FrameConverter::~FrameConverter()
{
}

bool FrameConverter::convert(webrtc::VideoFrameBuffer *srcBuffer, webrtc::I420Buffer *dstI420Buffer)
{
    int ret;

    if (srcBuffer->width() == dstI420Buffer->width() && srcBuffer->height() == dstI420Buffer->height()) {
        ret = libyuv::I420Copy(
                srcBuffer->DataY(), srcBuffer->StrideY(),
                srcBuffer->DataU(), srcBuffer->StrideU(),
                srcBuffer->DataV(), srcBuffer->StrideV(),
                dstI420Buffer->MutableDataY(), dstI420Buffer->StrideY(),
                dstI420Buffer->MutableDataU(), dstI420Buffer->StrideU(),
                dstI420Buffer->MutableDataV(), dstI420Buffer->StrideV(),
                dstI420Buffer->width(),        dstI420Buffer->height());
        if (ret != 0) {
            ELOG_ERROR("libyuv::I420Copy failed(%d)", ret);
            return false;
        }
    } else { // scale
        ret = libyuv::I420Scale(
                srcBuffer->DataY(),   srcBuffer->StrideY(),
                srcBuffer->DataU(),   srcBuffer->StrideU(),
                srcBuffer->DataV(),   srcBuffer->StrideV(),
                srcBuffer->width(),   srcBuffer->height(),
                dstI420Buffer->MutableDataY(),  dstI420Buffer->StrideY(),
                dstI420Buffer->MutableDataU(),  dstI420Buffer->StrideU(),
                dstI420Buffer->MutableDataV(),  dstI420Buffer->StrideV(),
                dstI420Buffer->width(),         dstI420Buffer->height(),
                libyuv::kFilterBox);
        if (ret != 0) {
            ELOG_ERROR("libyuv::I420Scale failed(%d)", ret);
            return false;
        }
    }

    return true;
}

}//namespace owt_base
