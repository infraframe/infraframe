// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var fs = require('fs');
var networkHelper = require('./networkHelper');
const dataAccess = require('./data_access');

module.exports.load = async function () {
  try {
    let conference_agent = await dataAccess.Config.findOne({name: 'conference_agent'});
    let config = Object.fromEntries(conference_agent.config.entries()) || {};
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

    config.internal.protocol = config.internal.protocol || 'tcp';

    config.mongo.databaseURL = config.mongo.databaseURL || 'localhost/owtdb';

    return config;
  } catch (e) {
    console.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
    process.exit(1);
  }

};
