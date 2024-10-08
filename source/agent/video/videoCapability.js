// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

const isHWAccAppliable = () => {
  // Query the hardware capability only if we want to try it.
  var info = '';
  try {
    info = require('child_process')
      .execSync('gst-inspect-1.0', {
        env: process.env,
        stdio: ['ignore', 'pipe', 'pipe'],
      })
      .toString();
  } catch (error) {
    if (error && error.code !== 0) {
      return false;
    } else {
      info = error.stderr.toString();
    }
  }
  return info.indexOf('nvvideo4linux2') != -1 ? true : false;
};

module.exports.detected = (requireHWAcc) => {
  var useHW = false;
  var codecs = {
    decode: ['vp8', 'vp9', 'av1', 'h265', 'h264'],
    encode: ['vp8', 'vp9', 'av1'],
  };

  if (isHWAccAppliable()) {
    // TODO: 设置为true会加载MSDK相关的内容。使用NV硬编解码器替代MSDK后，再设置为true
    // useHW = true;
    codecs.encode.push('h265');
    codecs.encode.push('h264_CB');
    codecs.encode.push('h264_B');
  }

  return {
    hw: useHW,
    codecs: codecs,
  };
};
