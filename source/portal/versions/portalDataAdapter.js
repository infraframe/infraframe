// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Portal's request/response data adapter for different versions
 */

"use strict";

const log = require("../logger").logger.getLogger("PortalDataAdapter");
const ReqType = require("./requestType");

module.exports = function (version) {
  const requestData = require("./requestDataValidator")(version);
  return {
    translateReq: function (type, req) {
      return requestData.validate(type, req).then((data) => {
        return data;
      });
    },
    translateResp: function (type, resp) {
      return resp;
    },
    translateNotification: function (evt, data) {
      return { evt, data };
    },
  };
};
