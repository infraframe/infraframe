// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AnalyticsPipeline_h
#define AnalyticsPipeline_h

#include <gst/gst.h>
#include <string>
#include <unordered_map>

#include "rvadefs.h"

/// Realtime Video Analytics Plugin interface definition shared by MCU and
/// plugin implementation.
class rvaPipeline {
public:
    /// Constructor
    rvaPipeline() { }
    /**
   @brief Initializes a plugin with provided params after MCU creates the plugin.
   @param params unordered map that contains name-value pair of parameters
   @return RVA_ERR_OK if no issue initialize it. Other return code if any failure.
  */
    virtual rvaStatus PipelineConfig(std::unordered_map<std::string, std::string> params) = 0;
    /**
   @brief Release internal resources the plugin holds before MCU destroy the plugin.
   @return RVA_ERR_OK if no issue close the plugin. Other return code if any failure.
  */
    virtual rvaStatus PipelineClose() = 0;
    /**
   @brief MCU will use this interface to fetch current applied params on the plugin.
   @param params name-value pair will be returned to the MCU provided unordered_map.
   @return RVA_ERR_OK if params are successfull filled in, or empty param is provided.
           Other return code if any failure.
  */
    virtual rvaStatus GetPipelineParams(std::unordered_map<std::string, std::string>& params) = 0;
    /**
   @brief MCU will use this interface to update params on the plugin.
   @param params name-value pair to be set.
   @return RVA_ERR_OK if params are successfull updated.
           Other return code if any failure.
  */
    virtual rvaStatus SetPipelineParams(std::unordered_map<std::string, std::string> params) = 0;

    virtual GstElement* InitializePipeline();

    virtual rvaStatus LinkElements();
};

typedef rvaPipeline* rva_create_t();
typedef void rva_destroy_t(rvaPipeline*);

/// Your plugin implemenation is required to invoke
/// below DECLARE_PLUGIN macro to delcare interfaces
/// to MCU.
#define DECLARE_PIPELINE(className)      \
    extern "C" {                         \
    rvaPipeline* CreatePipeline()        \
    {                                    \
        return new className;            \
    }                                    \
    void DestroyPipeline(rvaPipeline* p) \
    {                                    \
        delete p;                        \
    }                                    \
    }

#endif
