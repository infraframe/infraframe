// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var networkHelper = require('./networkHelper');
const dataAccess = require('./data_access');

module.exports.load = async function () {
  try {
    let audio_agent = await dataAccess.Config.findOne({name: 'audio_agent'});
    let config = Object.fromEntries(audio_agent.config.entries()) || {};

    config.agent = config.agent || {};
    config.agent.maxProcesses = config.agent.maxProcesses || -1;
    config.agent.prerunProcesses = config.agent.prerunProcesses || 2;

    config.cluster = config.cluster || {};
    config.cluster.name = config.cluster.name || 'owt-cluster';
    config.cluster.worker = config.cluster.worker || {};
    config.cluster.worker.ip = config.cluster.worker.ip || (networkHelper.getAddress("firstEnumerated") || {}).ip || 'unknown';
    config.cluster.worker.join_retry = config.cluster.worker.join_retry || 60;
    config.cluster.worker.load = config.cluster.worker.load || {};
    config.cluster.worker.load.max = config.cluster.max_load || 0.85;
    config.cluster.worker.load.period = config.cluster.report_load_interval || 1000;
    config.cluster.worker.load.item = {
      name: 'cpu'
    };

    config.capacity = config.capacity || {};

    config.internal.ip_address = config.internal.ip_address || '';
    config.internal.network_interface = config.internal.network_interface || undefined;
    config.internal.minport = config.internal.minport || 0;
    config.internal.maxport = config.internal.maxport || 0;

    if (!config.internal.ip_address) {
      let addr = networkHelper.getAddress(config.internal.network_interface || "firstEnumerated");
      if (!addr) {
        console.error("Can't get internal IP address");
        process.exit(1);
      }

      config.internal.ip_address = addr.ip;
    }

    config.mix = config.mix || {};
    config.mix.top_k = config.mix.top_k || 0;

    return config;
  } catch (e) {
    console.error('ERROR on read config: ' + e.message);
    process.exit(1);
  }

};
