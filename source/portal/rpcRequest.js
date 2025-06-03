// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const log = require('./logger').logger.getLogger('RpcRequest');
const GRPC_TIMEOUT = 3000;
const STREAM_ENGINE = global.config?.portal?.stream_engine_name || null;

var RpcRequest = function () {
  var that = {};
  let clusterClient;
  const grpcAgents = {}; // workerAgent => grpcClient
  const grpcNode = {}; // workerNode => grpcClient
  const opt = () => ({ deadline: new Date(Date.now() + GRPC_TIMEOUT) });

  const startConferenceClientIfNeeded = function (node) {
    if (grpcNode[node]) {
      return grpcNode[node];
    }
    log.debug('Start conference client:', node);
    grpcNode[node] = grpcTools.startClient('conference', node);
    // Add notification listener
    const handler = that.notificationHandler;
    if (handler) {
      const call = grpcNode[node].listenToNotifications({ id: 'portal' });
      call.on('data', (notification) => {
        log.debug('On notification data:', JSON.stringify(notification));
        if (notification.name === 'drop') {
          handler.drop(notification.id);
        } else {
          const data = JSON.parse(notification.data);
          if (notification.room) {
            handler.broadcast(
              notification.room,
              [notification.from],
              notification.name,
              data,
              () => {}
            );
          } else {
            handler.notify(notification.id, notification.name, data, () => {});
          }
        }
      });
      call.on('end', (err) => {
        log.warn('Listen notifications end:', err);

        grpcNode[node].close();
        delete grpcNode[node];
      });
      call.on('error', (err) => {
        log.warn('Listen notifications error:', err);
      });
    }
    return grpcNode[node];
  };

  that.getController = function (clusterManager, roomId, customizedPurpose) {
    if (STREAM_ENGINE) {
      return Promise.resolve(STREAM_ENGINE);
    }

    if (!clusterClient) {
      clusterClient = grpcTools.startClient('clusterManager', clusterManager);
    }
    let agentAddress;
    return new Promise((resolve, reject) => {
      const purpose = customizedPurpose ?? 'conference';
      const req = {
        purpose,
        task: roomId,
        preference: {}, // Change data for some preference
        reserveTime: 30 * 1000,
      };
      clusterClient.schedule(req, opt(), (err, result) => {
        if (err) {
          log.debug('Schedule node error:', err);
          reject(err);
        } else {
          resolve(result);
        }
      });
    }).then((workerAgent) => {
      agentAddress = workerAgent.info.ip + ':' + workerAgent.info.grpcPort;
      if (!grpcAgents[agentAddress]) {
        grpcAgents[agentAddress] = grpcTools.startClient(
          'nodeManager',
          agentAddress
        );
      }
      return new Promise((resolve, reject) => {
        const req = { info: { room: roomId, task: roomId } };
        grpcAgents[agentAddress].getNode(req, opt(), (err, result) => {
          if (!err) {
            const node = result.message;
            startConferenceClientIfNeeded(node);
            resolve(node);
          } else {
            reject(err);
          }
        });
      });
    });
  };

  that.join = function (controller, roomId, participant) {
    startConferenceClientIfNeeded(controller);
    const req = { roomId, participant };
    return new Promise((resolve, reject) => {
      grpcNode[controller].join(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.leave = function (controller, participantId) {
    startConferenceClientIfNeeded(controller);
    const req = { id: participantId };
    return new Promise((resolve, reject) => {
      grpcNode[controller].leave(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.text = function (controller, fromWhom, toWhom, message) {
    startConferenceClientIfNeeded(controller);
    const req = {
      from: fromWhom,
      to: toWhom,
      message,
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].text(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.publish = function (controller, participantId, streamId, Options) {
    startConferenceClientIfNeeded(controller);
    const req = {
      participantId,
      streamId,
      pubInfo: Options,
    };
    if (req.pubInfo.attributes) {
      req.pubInfo.attributes = JSON.stringify(req.pubInfo.attributes);
    }
    return new Promise((resolve, reject) => {
      grpcNode[controller].publish(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.unpublish = function (controller, participantId, streamId) {
    startConferenceClientIfNeeded(controller);
    const req = {
      participantId,
      sessionId: streamId,
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].unpublish(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.streamControl = function (controller, participantId, streamId, command) {
    startConferenceClientIfNeeded(controller);
    // To JSON command
    const req = {
      participantId,
      sessionId: streamId,
      command: JSON.stringify(command),
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].streamControl(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.subscribe = function (
    controller,
    participantId,
    subscriptionId,
    Options
  ) {
    startConferenceClientIfNeeded(controller);
    const req = {
      participantId,
      subscriptionId,
      subInfo: Options,
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].subscribe(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.unsubscribe = function (controller, participantId, subscriptionId) {
    startConferenceClientIfNeeded(controller);
    const req = {
      participantId,
      sessionId: subscriptionId,
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].unsubscribe(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.subscriptionControl = function (
    controller,
    participantId,
    subscriptionId,
    command
  ) {
    log.warn('subscriptionControl, ', participantId, subscriptionId, command);
    startConferenceClientIfNeeded(controller);
    const req = {
      participantId,
      sessionId: subscriptionId,
      command: JSON.stringify(command),
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].subscriptionControl(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  that.onSessionSignaling = function (
    controller,
    sessionId,
    signaling,
    participantId
  ) {
    startConferenceClientIfNeeded(controller);
    const req = {
      id: sessionId,
      signaling,
    };
    return new Promise((resolve, reject) => {
      grpcNode[controller].onSessionSignaling(req, opt(), (err, result) => {
        if (err) {
          reject(err.message);
        } else {
          resolve(result);
        }
      });
    });
  };

  return that;
};

module.exports = RpcRequest;
