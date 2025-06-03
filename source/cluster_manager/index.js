// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const logger = require('./logger').logger;
const log = logger.getLogger('Main');
const ClusterManager = require('./clusterManager');
const grpcTools = require('./grpcTools');
const dataAccess = require('./data_access');

async function main() {
  let clusterManager;
  let config;
  try {
    let cluster_manager = await dataAccess.Config.findOne({
      name: 'cluster_manager',
    });
    config = Object.fromEntries(cluster_manager.config.entries()) || {};
  } catch (e) {
    log.error('ERROR on read config: ' + e.message);
    process.exit(1);
  }

  config.manager = config.manager || {};
  config.manager.name = config.manager.name || 'owt-cluster';
  config.manager.initial_time = config.manager.initial_time || 10 * 1000;
  config.manager.check_alive_interval =
    config.manager.check_alive_interval || 1000;
  config.manager.check_alive_count = config.manager.check_alive_count || 10;
  config.manager.schedule_reserve_time =
    config.manager.schedule_reserve_time || 60 * 1000;
  config.manager.totalNode = config.manager.totalNode || 1;
  config.manager.electTimeout = config.manager.electTimeout || 1000;

  config.strategy = config.strategy || {};
  config.strategy.general = config.strategy.general || 'round-robin';
  config.strategy.portal = config.strategy.portal || 'last-used';
  config.strategy.conference = config.strategy.conference || 'last-used';
  config.strategy.webrtc = config.strategy.webrtc || 'last-used';
  config.strategy.sip = config.strategy.sip || 'round-robin';
  config.strategy.streaming = config.strategy.streaming || 'round-robin';
  config.strategy.recording = config.strategy.recording || 'randomly-pick';
  config.strategy.audio = config.strategy.audio || 'most-used';
  config.strategy.video = config.strategy.video || 'least-used';
  config.strategy.analytics = config.strategy.analytics || 'least-used';

  function startup() {
    var id = Math.floor(Math.random() * 1000000000);
    var spec = {
      initialTime: config.manager.initial_time,
      checkAlivePeriod: config.manager.check_alive_interval,
      checkAliveCount: config.manager.check_alive_count,
      scheduleKeepTime: config.manager.schedule_reserve_time,
      strategy: config.strategy,
      totalNode: config.manager.totalNode,
      electTimeout: config.manager.electTimeout,
    };

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
}

main().catch((err) => console.log(err));
