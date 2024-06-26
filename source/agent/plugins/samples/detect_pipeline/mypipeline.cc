// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "mypipeline.h"
#include "toml.hpp"
#include <iostream>
#include <string.h>

MyPipeline::MyPipeline()
{
    inputwidth = 640;
    inputheight = 480;
    inputframerate = 24;
    pipeline = NULL;
    source = NULL;
    fakesink = NULL;
    std::cout << "MyPipeline constructor" << std::endl;
}

rvaStatus MyPipeline::PipelineConfig(std::unordered_map<std::string, std::string> params)
{
    std::cout << "In my plugin init." << std::endl;

    std::unordered_map<std::string, std::string>::const_iterator width = params.find("inputwidth");
    if (width == params.end())
        std::cout << "inputwidth is not found" << std::endl;
    else
        inputwidth = std::atoi(width->second.c_str());

    std::unordered_map<std::string, std::string>::const_iterator height = params.find("inputheight");
    if (height == params.end())
        std::cout << "inputheight is not found" << std::endl;
    else
        inputheight = std::atoi(height->second.c_str());

    std::unordered_map<std::string, std::string>::const_iterator framerate = params.find("inputframerate");
    if (framerate == params.end())
        std::cout << "inputframerate is not found" << std::endl;
    else
        inputframerate = std::atoi(framerate->second.c_str());

    std::unordered_map<std::string, std::string>::const_iterator name = params.find("pipelinename");
    if (name == params.end())
        std::cout << "pipeline is not found" << std::endl;
    else
        pipelinename = name->second;

    return RVA_ERR_OK;
}

rvaStatus MyPipeline::PipelineClose()
{
    return RVA_ERR_OK;
}

GstElement* MyPipeline::InitializePipeline()
{
    /* Initialize GStreamer */
    gst_init(NULL, NULL);

    std::cout << "Start intialize elements" << std::endl;
    /* Create the elements */
    source = gst_element_factory_make("appsrc", "appsource");
    h264parse = gst_element_factory_make("h264parse", "parse");
    decodebin = gst_element_factory_make("vaapih264dec", "decode");
    postproc = gst_element_factory_make("vaapipostproc", "postproc");
    detect = gst_element_factory_make("gvadetect", "detect");
    counter = gst_element_factory_make("gvafpscounter", "counter");
    fakesink = gst_element_factory_make("fakesink", "fake");
    videorate = gst_element_factory_make("videorate", "rate");

    pipeline = gst_pipeline_new("detect-pipeline");

    if (!detect) {
        std::cout << "detect element coule not be created" << std::endl;
        return NULL;
    }

    if (!pipeline || !source || !decodebin || !h264parse || !postproc || !videorate) {
        std::cout << "pipeline or source or decodebin or h264parse or postproc elements could not be created." << std::endl;
        return NULL;
    }

    return pipeline;
}

rvaStatus MyPipeline::LinkElements()
{
    gboolean link_ok;
    GstCaps *postprocsrccaps, *postprocsinkcaps;
    const char* path = std::getenv("CONFIGFILE_PATH");
    const auto data = toml::parse(path);
    const auto& pipelineconfig = toml::find(data, pipelinename.c_str());
    const auto model = toml::find<std::string>(pipelineconfig, "modelpath");
    const auto inferencewidth = toml::find<std::int64_t>(pipelineconfig, "inferencewidth");
    const auto inferenceheight = toml::find<std::int64_t>(pipelineconfig, "inferenceheight");
    const auto inferenceframerate = toml::find<std::int64_t>(pipelineconfig, "inferenceframerate");
    const auto device = toml::find<std::string>(pipelineconfig, "device");

    std::cout << "inferencewidth is:" << inferencewidth << std::endl;
    std::cout << "inferenceheight is:" << inferenceheight << std::endl;
    std::cout << "inferenceframerate is:" << inferenceframerate << std::endl;
    std::cout << "model is:" << model << std::endl;
    std::cout << "inputwidth is:" << inputwidth << std::endl;
    std::cout << "input height is:" << inputheight << std::endl;
    std::cout << "input framerate is:" << inputframerate << std::endl;

    postprocsinkcaps = gst_caps_from_string("video/x-raw(memory:VASurface),format=NV12");
    postprocsrccaps = gst_caps_from_string("video/x-raw(memory:VASurface),format=NV12");

    GstCaps* inputcaps = gst_caps_new_simple("video/x-h264",
        "format", G_TYPE_STRING, "avc",
        "width", G_TYPE_INT, inputwidth,
        "height", G_TYPE_INT, inputheight,
        "framerate", GST_TYPE_FRACTION, inputframerate, 1, NULL);

    g_object_set(source, "caps", inputcaps, NULL);
    gst_caps_unref(inputcaps);

    g_object_set(G_OBJECT(videorate), "max-rate", inferenceframerate, NULL);
    g_object_set(G_OBJECT(postproc), "brightness", 0.0001, NULL);

    g_object_set(G_OBJECT(detect), "device", device.c_str(),
        "model", model.c_str(),
        "cpu-streams", 12,
        "nireq", 24,
        "pre-proc", "vaapi",
        "inference-id", "dtc", NULL);

    g_object_set(G_OBJECT(fakesink), "async", false, NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, decodebin, videorate, postproc, h264parse, detect, counter, fakesink, NULL);

    if (gst_element_link_many(source, h264parse, decodebin, videorate, NULL) != TRUE) {
        std::cout << "Elements source,decodebin could not be linked." << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }

    link_ok = gst_element_link_filtered(videorate, postproc, postprocsinkcaps);
    gst_caps_unref(postprocsinkcaps);

    if (!link_ok) {
        std::cout << "Failed to link videorate and postproc!" << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }

    link_ok = gst_element_link_filtered(postproc, detect, postprocsrccaps);
    gst_caps_unref(postprocsrccaps);

    if (!link_ok) {
        std::cout << "Failed to link postproc and detect!" << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }

    if (gst_element_link_many(detect, counter, fakesink, NULL) != TRUE) {
        std::cout << "Elements detect,counter,fakesink could not be linked. " << std::endl;
        gst_object_unref(pipeline);
        return RVA_ERR_LINK;
    }

    return RVA_ERR_OK;
}

// Declare the pipeline
DECLARE_PIPELINE(MyPipeline)
