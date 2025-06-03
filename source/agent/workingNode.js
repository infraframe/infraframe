// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var logger = require('./logger').logger;
var log = logger.getLogger('WorkingNode');

var cxxLogger;
try {
  cxxLogger = require('./logger/build/Release/logger');
} catch (e) {
  log.debug('No native logger for reconfiguration');
}

var controller;
function init_controller() {
  log.info('pid:', process.pid);

  var nodeConfig = JSON.parse(process.argv[4]);
  var rpcID = process.argv[2];
  var parentID = process.argv[3];
  var purpose = nodeConfig.purpose;
  var clusterWorkerIP = nodeConfig.cluster.worker.ip;

  global.config = nodeConfig;

  log.info('Starting grpc server');
  const checkAlivePeriod = 1000;
  const mockClient = {
    remoteCast: (to, method, args) => {
      log.debug('Mock remote cast:', to, method, args);
    },
    remoteCall: (to, method, args) => {
      log.debug('Mock remote call:', to, method, args);
    },
  };
  controller = require('./' + purpose)(
    mockClient,
    rpcID,
    parentID,
    clusterWorkerIP
  );
  const grpcTools = require('./grpcTools');
  grpcTools
    .startServer(purpose, controller.grpcInterface)
    .then((port) => {
      // Send RPC server address
      const rpcAddress = clusterWorkerIP + ':' + port;
      mockClient.rpcAddress = rpcAddress;
      process.send('READY:' + rpcAddress);
      setInterval(() => {
        process.send('IMOK');
      }, checkAlivePeriod);
    })
    .catch((err) => {
      log.error('Start grpc server failed:', err);
      process.send('ERROR');
    });
}

var exiting = false;
['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, async function () {
    if (exiting) {
      return;
    }
    exiting = true;
    log.warn('Exiting on', sig);
    if (controller && typeof controller.close === 'function') {
      controller.close();
    }
    try {
      await rpc.disconnect();
    } catch (e) {
      log.warn('Exiting e:', e);
    }
    process.exit();
  });
});

['SIGHUP', 'SIGPIPE'].map(function (sig) {
  process.on(sig, function () {
    log.warn(sig, 'caught and ignored');
  });
});

process.on('exit', function () {
  log.info('Process exit');
});

process.on('uncaughtException', async (err) => {
  log.error(err, err.stack);
  process.exit(1);
});

process.on('unhandledRejection', (reason) => {
  log.info('Reason: ', reason.stack);
});

process.on('SIGUSR2', function () {
  logger.reconfigure();
  if (cxxLogger) {
    cxxLogger.configure();
  }
});

(function main() {
  init_controller();
})();
