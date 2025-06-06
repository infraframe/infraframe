// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpcTools = require('./grpcTools');
const packOption = grpcTools.packOption;
const unpackNotification = grpcTools.unpackNotification;

const makeRPC = require('./makeRPC').makeRPC;
const log = require('./logger').logger.getLogger('RpcRequest');

const GRPC_TIMEOUT = 3000;

const RpcRequest = function (listener) {
  const that = {};
  const grpcAgents = {}; // workerAgent => grpcClient
  const grpcNode = {}; // workerNode => grpcClient
  const nodeType = {}; // NodeId => Type
  let clusterClient;
  let sipPortalClient;
  const opt = () => ({ deadline: new Date(Date.now() + GRPC_TIMEOUT) });

  that.getWorkerNode = function (clusterManager, purpose, forWhom, preference) {
    log.debug('getworker node:', purpose, forWhom, clusterManager);
    if (!clusterClient) {
      clusterClient = grpcTools.startClient('clusterManager', clusterManager);
    }
    let agentAddress;
    return new Promise((resolve, reject) => {
      const req = {
        purpose,
        task: forWhom.task,
        preference, // Change data for some preference
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
    })
      .then((workerAgent) => {
        agentAddress = workerAgent.info.ip + ':' + workerAgent.info.grpcPort;
        if (!grpcAgents[agentAddress]) {
          grpcAgents[agentAddress] = grpcTools.startClient(
            'nodeManager',
            agentAddress
          );
        }
        return new Promise((resolve, reject) => {
          grpcAgents[agentAddress].getNode(
            { info: forWhom },
            opt(),
            (err, result) => {
              if (!err) {
                resolve(result.message);
              } else {
                reject(err);
              }
            }
          );
        });
      })
      .then((workerNode) => {
        if (grpcNode[workerNode]) {
          // Has client already
          return { agent: agentAddress, node: workerNode };
        }
        log.debug('Start gRPC client:', purpose, workerNode);
        grpcNode[workerNode] = grpcTools.startClient(purpose, workerNode);
        nodeType[workerNode] = purpose;
        // Register listener
        const call = grpcNode[workerNode].listenToNotifications({ id: '' });
        call.on('data', (notification) => {
          if (listener) {
            // Unpack notification.data
            const data = unpackNotification(notification);
            if (data) {
              listener.processNotification(data);
            }
          }
        });
        call.on('end', (err) => {
          log.debug('Call on end:', err);
          if (grpcNode[workerNode]) {
            grpcNode[workerNode].close();
            delete grpcNode[workerNode];
          }
        });
        call.on('error', (err) => {
          // On error
          log.debug('Call on error:', err);
        });
        return { agent: agentAddress, node: workerNode };
      });
  };

  that.recycleWorkerNode = function (workerAgent, workerNode, forWhom) {
    if (grpcAgents[workerAgent]) {
      const req = { id: workerNode, info: forWhom };
      return new Promise((resolve, reject) => {
        grpcAgents[workerAgent].recycleNode(req, opt(), (err, result) => {
          if (err) {
            log.debug('Recycle node error:', err);
            reject(err);
          }
          if (grpcNode[workerNode]) {
            grpcNode[workerNode].close();
            delete grpcNode[workerNode];
            delete nodeType[workerNode];
          }
          resolve('ok');
        });
      });
    }
  };

  that.initiate = function (
    accessNode,
    sessionId,
    sessionType,
    direction,
    Options
  ) {
    if (grpcNode[accessNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        if (direction === 'in') {
          const req = {
            id: sessionId,
            type: sessionType,
            option: packOption(sessionType, Options),
          };
          grpcNode[accessNode].publish(req, opt(), (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else if (direction === 'out') {
          const req = {
            id: sessionId,
            type: sessionType,
            option: packOption(sessionType, Options),
          };
          grpcNode[accessNode].subscribe(req, opt(), (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else {
          reject('Invalid direction');
        }
      });
    }
  };

  that.terminate = function (
    accessNode,
    sessionId,
    direction /*FIXME: direction should be unneccesarry*/
  ) {
    if (grpcNode[accessNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        if (direction === 'in') {
          const req = { id: sessionId };
          grpcNode[accessNode].unpublish(req, opt(), (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else if (direction === 'out') {
          const req = { id: sessionId };
          grpcNode[accessNode].unsubscribe(req, opt(), (err, result) => {
            if (!err) {
              resolve(result);
            } else {
              reject(err);
            }
          });
        } else {
          reject('Invalid direction');
        }
      });
    }
  };

  that.onTransportSignaling = function (accessNode, transportId, signaling) {
    // GRPC for webrtc-agent
    if (grpcNode[accessNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        const req = {
          id: transportId,
          signaling: signaling,
        };
        grpcNode[accessNode].processSignaling(req, opt(), (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
  };

  that.destroyTransport = function (accessNode, transportId) {
    // GRPC for webrtc-agent
    if (grpcNode[accessNode]) {
      // Use GRPC
      const req = { id: transportId };
      return new Promise((resolve, reject) => {
        grpcNode[accessNode].destroyTransport(req, opt(), (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
  };

  that.mediaOnOff = function (accessNode, sessionId, track, direction, onOff) {
    if (grpcNode[accessNode]) {
      // Use GRPC
      const req = {
        id: sessionId,
        tracks: track,
        direction: direction,
        action: onOff,
      };
      return new Promise((resolve, reject) => {
        grpcNode[accessNode].sessionControl(req, opt(), (err, result) => {
          if (!err) {
            resolve(result);
          } else {
            reject(err);
          }
        });
      });
    }
  };

  that.getRoomConfig = function (configServer, sessionId) {
    return Promise.resolve();
  };

  that.getSipConnectivity = function (sipPortal, roomId) {
    if (!sipPortalClient) {
      const sipPortal = global.config.cluster.sip_portal || 'localhost:9090';
      sipPortalClient = startClient('sipPortal', sipPortal);
    }
    return new Promise((resolve, reject) => {
      sipPortalClient.getSipConnectivity(
        { id: roomId },
        opt(),
        (err, result) => {
          if (!err) {
            const sipNode = result.message;
            grpcNode[sipNode] = grpcTools.startClient('sip', sipNode);
            resolve(sipNode);
          } else {
            reject(err);
          }
        }
      );
    });
  };

  that.makeSipCall = function (
    sipNode,
    peerURI,
    mediaIn,
    mediaOut,
    controller
  ) {
    if (grpcNode[sipNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        const req = {
          peerUri: peerURI,
          mediaIn: mediaIn,
          mediaOut: mediaOut,
          controller: controller,
        };
        grpcNode[sipNode].makeCall(req, opt(), (err, result) => {
          if (!err) {
            resolve(result.message);
          } else {
            reject(err);
          }
        });
      });
    }
  };

  that.endSipCall = function (sipNode, sipCallId) {
    if (grpcNode[sipNode]) {
      // Use GRPC
      return new Promise((resolve, reject) => {
        grpcNode[sipNode].endSipCall(
          { id: sipCallId },
          opt(),
          (err, result) => {
            if (!err) {
              resolve(result.message);
            } else {
              reject(err);
            }
          }
        );
      });
    }
  };

  that.createLayerStreams = function (accessNode, trackId, preferredLayers) {
    return Promise.resolve();
  };

  that.createQualitySwitch = function (accessNode, froms) {
    return Promise.resolve();
  };

  that.getClusterID = function (clusterManager, room_id, roomToken) {
    return Promise.resolve();
  };

  that.leaveConference = function (clusterManager, roomId) {
    return Promise.resolve();
  };

  that.sendMsg = function (portal, participantId, event, data) {
    return Promise.resolve();
  };

  that.broadcast = function (portal, controller, excludeList, event, data) {
    return Promise.resolve();
  };

  that.dropUser = function (portal, participantId) {
    return Promise.resolve();
  };

  that.makeRPC = function (_, remoteNode, rpcName, parameters, onOk, onError) {
    if (grpcNode[remoteNode]) {
      if (rpcName === 'linkup') {
        const req = {
          id: parameters[0],
          from: parameters[1],
        };
        grpcNode[remoteNode].linkup(req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'cutoff') {
        const req = { id: parameters[0] };
        grpcNode[remoteNode].cutoff(req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'getInternalAddress') {
        grpcNode[remoteNode].getInternalAddress({}, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'publish' || rpcName === 'subscribe') {
        const direction = rpcName === 'publish' ? 'in' : 'out';
        that
          .initiate(
            remoteNode,
            parameters[0],
            parameters[1],
            direction,
            parameters[2]
          )
          .then((result) => {
            onOk && onOk(result);
          })
          .catch((err) => {
            onError && onError(err);
          });
      } else if (rpcName === 'unpublish' || rpcName === 'unsubscribe') {
        const direction = rpcName === 'unpublish' ? 'in' : 'out';
        that
          .terminate(remoteNode, parameters[0], direction)
          .then((result) => {
            onOk && onOk(result);
          })
          .catch((err) => {
            onError && onError(err);
          });
      } else if (rpcName === 'init') {
        const type = nodeType[remoteNode];
        const initOption = {
          service: parameters[0],
          controller: parameters[3],
          label: parameters[4],
          init: parameters[1],
        };
        const req = {
          id: parameters[2],
          type: type,
          option: packOption(type, initOption),
        };
        grpcNode[remoteNode].init(req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'generate') {
        const req = {
          id: parameters[0],
          media: {},
        };
        if (parameters.length === 2) {
          req.media.audio = { format: { codec: parameters[1] } };
        } else if (parameters.length === 5) {
          parameters = parameters.map((par) =>
            par === 'unspecified' ? undefined : par
          );
          req.media.video = {
            format: { codec: parameters[0] },
            parameters: {
              resolution: parameters[1],
              framerate: parameters[2],
              bitrate: parameters[3],
              keyFrameInterval: parameters[4],
            },
          };
        }
        grpcNode[remoteNode].generate(req, opt(), (err, result) => {
          if (!err) {
            const resp = req.media.audio ? result.id : result;
            onOk && onOk(resp);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'degenerate') {
        const req = { id: parameters[0] };
        grpcNode[remoteNode].degenerate(req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'enableVAD') {
        const req = { periodMs: parameters[0] };
        grpcNode[remoteNode].enableVad(req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'resetVAD' || rpcName === 'deinit') {
        const req = {};
        if (rpcName === 'resetVAD') {
          rpcName = 'resetVad';
        }
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setInputActive') {
        const req = {
          id: parameters[0],
          active: parameters[1],
        };
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'getRegion') {
        const req = { id: parameters[0] };
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result.message);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setRegion') {
        const req = {
          streamId: parameters[0],
          regionId: parameters[1],
        };
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setLayout') {
        const req = { regions: parameters[0] };
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result.regions);
          } else {
            onError && onError(err);
          }
        });
      } else if (rpcName === 'setPrimary' || rpcName === 'forceKeyFrame') {
        const req = { id: parameters[0] };
        grpcNode[remoteNode][rpcName](req, opt(), (err, result) => {
          if (!err) {
            onOk && onOk(result);
          } else {
            onError && onError(err);
          }
        });
      } else {
        log.error('Unknown rpc name:', rpcName);
      }
    } else {
      log.warn('Unknown gRPC node:', remoteNode);
      return makeRPC(_, remoteNode, rpcName, parameters, onOk, onError);
    }
  };

  that.addSipNode = function (workerNode) {
    if (enableGrpc) {
      grpcNode[workerNode] = grpcTools.startClient('sip', workerNode);
    }
  };

  return that;
};

module.exports = RpcRequest;
