// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MYPIPELINE_H
#define MYPIPELINE_H

#include "pipeline.h"
#include <gst/gst.h>

// Class definition for the pipeline invoked by the sample service.
class MyPipeline : public rvaPipeline {
public:
    MyPipeline();

    virtual rvaStatus PipelineConfig(std::unordered_map<std::string, std::string> params);

    virtual rvaStatus PipelineClose();

    virtual rvaStatus GetPipelineParams(std::unordered_map<std::string, std::string>& params)
    {
        return RVA_ERR_OK;
    }

    virtual rvaStatus SetPipelineParams(std::unordered_map<std::string, std::string> params)
    {
        return RVA_ERR_OK;
    }

    virtual GstElement* InitializePipeline();

    virtual rvaStatus LinkElements();

private:
    GstElement *pipeline, *source, *receive, *detect, *decodebin, *postproc, *h264parse, *counter, *fakesink, *videorate;
    int inputwidth, inputheight, inputframerate;
    std::string pipelinename;
};

#endif //MYPIPELINE_H
