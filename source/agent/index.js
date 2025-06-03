'use strict';
const Getopt = require('node-getopt');
const logger = require('./logger').logger;
const log = logger.getLogger('WorkingAgent');
const grpcTools = require('./grpcTools');

async function main() {
  let config;
  try {
    config = await require('./configLoader').load();
  } catch (e) {
    console.error(e);
    exit(1);
  }

  // Parse command line arguments
  let getopt = new Getopt([
    ['U', 'my-purpose=ARG', 'Purpose of this agent'],
    ['h', 'help', 'display this help'],
  ]);

  let myId = '';
  let myPurpose = 'webrtc';
  let myState = 2;

  let opt = getopt.parse(process.argv.slice(2));

  for (let prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
      let value = opt.options[prop];
      switch (prop) {
        case 'help':
          getopt.showHelp();
          process.exit(0);
          break;
        case 'my-purpose':
          myPurpose = value;
          break;
        default:
          break;
      }
    }
  }

  let clusterWorker = require('./clusterWorker');
  let nodeManager = require('./nodeManager');
  let rpcClient;
  let monitoringTarget;

  let worker;
  let manager;
  let grpcPort;

  let joinCluster = function (on_ok) {
    let joinOK = function (id) {
      myId = id;
      myState = 2;
      log.info(myPurpose, 'agent join cluster ok.');
      on_ok(id);
    };

    let joinFailed = function (reason) {
      log.error(myPurpose, 'agent join cluster failed. reason:', reason);
      process.exit();
    };

    let loss = function () {
      log.info(myPurpose, 'agent lost.');
      manager && manager.dropAllNodes(false);
    };

    let recovery = function () {
      log.info(myPurpose, 'agent recovered.');
      manager && manager.recover();
    };

    let overload = function () {
      log.warn(myPurpose, 'agent overloaded!');
      if (myPurpose === 'recording') {
        manager && manager.dropAllNodes(false);
      }
    };

    worker = clusterWorker({
      rpcClient: rpcClient,
      purpose: myPurpose,
      clusterName: config.cluster.name,
      joinRetry: config.cluster.worker.join_retry,
      // Cannot find a defination about |info|. It looks like it will be used by cluster manager, but agents and portal may have different properties of |info|.
      info: {
        ip: config.cluster.worker.ip,
        hostname: config[myPurpose] ? config[myPurpose].hostname : undefined,
        port: config[myPurpose] ? config[myPurpose].port : undefined,
        purpose: myPurpose,
        state: 2,
        maxLoad: config.cluster.worker.load.max,
        capacity: config.capacity,
        grpcPort: grpcPort,
      },
      onJoinOK: joinOK,
      onJoinFailed: joinFailed,
      onLoss: loss,
      onRecovery: recovery,
      onOverload: overload,
      loadCollection: config.cluster.worker.load,
    });
  };

  let init_manager = () => {
    let reuseNode = !(
      myPurpose === 'audio' ||
      myPurpose === 'video' ||
      myPurpose === 'analytics' ||
      myPurpose === 'conference' ||
      myPurpose === 'sip'
    );
    let consumeNodeByRoom = !(
      myPurpose === 'audio' ||
      myPurpose === 'video' ||
      myPurpose === 'analytics'
    );

    let spawnOptions = {
      cmd: 'node',
      config: Object.assign({}, config),
    };

    spawnOptions.config.purpose = myPurpose;

    manager = nodeManager(
      {
        parentId: myId,
        prerunNodeNum: config.agent.prerunProcesses,
        maxNodeNum: config.agent.maxProcesses,
        reuseNode: reuseNode,
        consumeNodeByRoom: consumeNodeByRoom,
      },
      spawnOptions,
      (nodeId, tasks) => {
        monitoringTarget &&
          monitoringTarget.notify('abnormal', {
            purpose: myPurpose,
            id: nodeId,
            type: 'node',
          });
        tasks.forEach((task) => {
          worker && worker.removeTask(task);
        });
      },
      (task) => {
        worker && worker.addTask(task);
      },
      (task) => {
        worker && worker.removeTask(task);
      }
    );
  };

  let rpcAPI = function (worker) {
    return {
      getNode: function (task, callback) {
        if (manager) {
          return manager
            .getNode(task)
            .then((nodeId) => {
              callback('callback', nodeId);
            })
            .catch((err) => {
              callback('callback', 'error', err);
            });
        } else {
          callback('callback', 'error', 'agent not ready');
        }
      },

      recycleNode: function (id, task, callback) {
        if (manager) {
          return manager
            .recycleNode(id, task)
            .then(() => {
              callback('callback', 'ok');
            })
            .catch((err) => {
              callback('callback', 'error', err);
            });
        } else {
          callback('callback', 'error', 'agent not ready');
        }
      },

      queryNode: function (task, callback) {
        if (manager) {
          return manager
            .queryNode(task)
            .then((nodeId) => {
              callback('callback', nodeId);
            })
            .catch((err) => {
              callback('callback', 'error', err);
            });
        } else {
          callback('callback', 'error', 'agent not ready');
        }
      },
    };
  };

  // Adapt MQ callback to grpc callback
  const cbAdapter = function (callback) {
    return function (n, code, data) {
      if (code === 'error') {
        callback(new Error(data), null);
      } else {
        callback(null, { message: code });
      }
    };
  };

  const grpcAPI = function (rpcAPI) {
    return {
      getNode: function (call, callback) {
        rpcAPI.getNode(call.request.info, cbAdapter(callback));
      },
      recycleNode: function (call, callback) {
        rpcAPI.recycleNode(
          call.request.id,
          call.request.info,
          cbAdapter(callback)
        );
      },
      queryNode: function (call, callback) {
        rpcAPI.queryNode(call.request.info.task, cbAdapter(callback));
      },
    };
  };

  grpcTools
    .startServer('nodeManager', grpcAPI(rpcAPI(worker)))
    .then((port) => {
      // Save GRPC port
      grpcPort = port;
      log.info('as rpc server ok', config.cluster.worker.ip + ':' + grpcPort);
      joinCluster(function (rpcId) {
        init_manager();
      });
    })
    .catch((err) => {
      log.error('Start grpc server failed:', err);
    });

  ['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
      log.warn('Exiting on', sig);
      manager && manager.dropAllNodes(true);
      worker && worker.quit();
      process.exit();
    });
  });

  process.on('exit', function () {
    log.info('Process exit');
  });

  process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
  });

  process.on('SIGUSR2', function () {
    logger.reconfigure();
  });
}

main().catch((err) => console.log(err));
