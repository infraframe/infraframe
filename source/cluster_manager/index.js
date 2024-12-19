// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('Main');
var ClusterManager = require('./clusterManager');
var fs = require('fs');
var config;
var clusterManager;
try {
  config = {
    agent: {
      maxProcesses: -1,
      prerunProcessed: 2,
    },
    manager: {
      name: 'owt-cluster',
      initial_time: 6000,
      check_alive_interval: 1000,
      check_alive_count: 3,
      schedule_reserve_time: 60000,
      grpc_host: 'localhost:10080',
    },
    strategy: {
      general: 'last-used',
      analytics: 'least-used',
      portal: 'last-used',
      conference: 'last-used',
      webrtc: 'last-used',
      streaming: 'round-robin',
      recording: 'randomly-pick',
      audio: 'most-used',
      video: 'least-used',
    },
  };
} catch (e) {
  log.error(
    'Parsing config error on line ' +
      e.line +
      ', column ' +
      e.column +
      ': ' +
      e.message
  );
  process.exit(1);
}

config.manager.name = config.manager.name || 'owt-cluster';
config.manager.initial_time = config.manager.initial_time || 10 * 1000;
config.manager.check_alive_interval =
  config.manager.check_alive_interval || 1000;
config.manager.check_alive_count = config.manager.check_alive_count || 10;
config.manager.schedule_reserve_time =
  config.manager.schedule_reserve_time || 60 * 1000;

config.strategy = config.strategy || {};
config.strategy.general = config.strategy.general || 'round-robin';
config.strategy.portal = config.strategy.portal || 'last-used';
config.strategy.conference = config.strategy.conference || 'last-used';
config.strategy.webrtc = config.strategy.webrtc || 'last-used';
config.strategy.streaming = config.strategy.streaming || 'round-robin';
config.strategy.recording = config.strategy.recording || 'randomly-pick';
config.strategy.audio = config.strategy.audio || 'most-used';
config.strategy.video = config.strategy.video || 'least-used';
config.strategy.analytics = config.strategy.analytics || 'least-used';

config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

config.cascading = config.cascading || {};
config.cascading.enabled = config.cascading.enabled || false;
config.cascading.url = config.cascading.url;
config.cascading.region = config.cascading.region;
config.cascading.clusterID = config.cascading.clusterID;

function startup() {
  var id = Math.floor(Math.random() * 1000000000);
  var spec = {
    initialTime: config.manager.initial_time,
    checkAlivePeriod: config.manager.check_alive_interval,
    checkAliveCount: config.manager.check_alive_count,
    scheduleKeepTime: config.manager.schedule_reserve_time,
    strategy: config.strategy,
    enableCascading: config.cascading.enabled,
    url: config.cascading.url,
    region: config.cascading.region,
    clusterID: config.cascading.clusterID,
  };

  const grpcTools = require('./grpcTools');
  let gHostname = 'localhost';
  let gPort = 10080;
  if (config.manager.grpc_host) {
    [gHostname, gPort] = config.manager.grpc_host.split(':');
    gPort = Number(gPort) || 10080;
  }
  const manager = new ClusterManager.ClusterManager(
    config.manager.name,
    id,
    spec
  );
  manager.serve();
  grpcTools
    .startServer('clusterManager', manager.grpcInterface, gPort)
    .then((port) => {
      // Send RPC server address
      const rpcAddress = gHostname + ':' + port;
      log.info('As gRPC server ok', rpcAddress);
    })
    .catch((err) => {
      log.error('Start grpc server failed:', err);
    });
  return;
}

startup();

['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, async function () {
    log.warn('Exiting on', sig);
    try {
      clusterManager.leave();
    } catch (e) {
      log.warn('Disconnect:', e);
    }
    process.exit();
  });
});

process.on('exit', function () {
  log.info('Process exit');
});

process.on('SIGUSR2', function () {
  logger.reconfigure();
});
