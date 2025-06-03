'use strict';
const logger = require('./logger').logger;
const log = logger.getLogger('Main');
const dataAccess = require('./data_access');

async function main() {
  let config;
try {
    let portal = await dataAccess.Config.findOne({name: 'portal'});
    config = Object.fromEntries(portal.config.entries()) || {};
} catch (e) {
    log.error('ERROR on read config: ' + e.message);
  process.exit(1);
}

// Configuration default values
config.portal = config.portal || {};
config.portal.ip_address = config.portal.ip_address || '';
config.portal.hostname = config.portal.hostname|| '';
config.portal.port = config.portal.port || 8080;
config.portal.via_host = config.portal.via_host || '';
config.portal.ssl = config.portal.ssl || false;
config.portal.force_tls_v12 = config.portal.force_tls_v12 || false;
config.portal.reconnection_ticket_lifetime = config.portal.reconnection_ticket_lifetime || 600;
config.portal.reconnection_timeout = Number.isInteger(config.portal.reconnection_timeout) ? config.portal.reconnection_timeout : 60;
config.portal.cors = config.portal.cors || [];

config.cluster = config.cluster || {};
config.cluster.name = config.cluster.name || 'owt-cluster';
config.cluster.grpc_host = config.cluster.grpc_host || 'localhost:10080';
config.cluster.join_retry = config.cluster.join_retry || 60;
config.cluster.report_load_interval = config.cluster.report_load_interval || 1000;
config.cluster.max_load = config.cluster.max_load || 0.85;
config.cluster.network_max_scale = config.cluster.network_max_scale || 1000;

config.capacity = config.capacity || {};
config.capacity.isps = config.capacity.isps || [];
config.capacity.regions = config.capacity.regions || [];

// Parse portal hostname and ip_address variables from ENV.
if (config.portal.ip_address.indexOf('$') == 0) {
    config.portal.ip_address = process.env[config.portal.ip_address.substr(1)];
    log.info('ENV: config.portal.ip_address=' + config.portal.ip_address);
}
if (config.portal.hostname.indexOf('$') == 0) {
    config.portal.hostname = process.env[config.portal.hostname.substr(1)];
    log.info('ENV: config.portal.hostname=' + config.portal.hostname);
}
if(process.env.owt_via_host !== undefined) {
    config.portal.via_host = process.env.owt_via_host;
    log.info('ENV: config.portal.via_address=' + config.portal.via_host);
}

global.config = config;

var rpcClient;
var socketio_server;
var portal;
var worker;

var ip_address;
(function getPublicIP() {
  var BINDED_INTERFACE_NAME = config.portal.networkInterface;
  var interfaces = require('os').networkInterfaces(),
    addresses = [],
    k,
    k2,
    address;

  for (k in interfaces) {
    if (interfaces.hasOwnProperty(k)) {
      for (k2 in interfaces[k]) {
        if (interfaces[k].hasOwnProperty(k2)) {
          address = interfaces[k][k2];
          if (address.family === 'IPv4' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push(address.address);
            }
          }
          if (address.family === 'IPv6' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push('[' + address.address + ']');
            }
          }
        }
      }
    }
  }

  if (config.portal.ip_address === '' || config.portal.ip_address === undefined){
    ip_address = addresses[0];
  } else {
    ip_address = config.portal.ip_address;
  }
})();

var dropAll = function() {
  socketio_server && socketio_server.drop('all');
};

var joinCluster = function (on_ok) {
  var joinOK = on_ok;

  var joinFailed = function (reason) {
    log.error('portal join cluster failed. reason:', reason);
    worker && worker.quit();
    process.exit();
  };

  var loss = function () {
    log.info('portal lost.');
    dropAll();
  };

  var recovery = function () {
    log.info('portal recovered.');
  };

  var spec = {rpcClient: rpcClient,
              purpose: 'portal',
              clusterName: config.cluster.name,
              joinRetry: config.cluster.join_retry,
              info: {
                ip: ip_address,
                hostname: config.portal.hostname,
                port: config.portal.port,
                via_host: config.portal.via_host,
                ssl: config.portal.ssl,
                state: 2,
                maxLoad: config.cluster.max_load,
                capacity: config.capacity,
                  grpcPort: 1,
              },
              onJoinOK: joinOK,
              onJoinFailed: joinFailed,
              onLoss: loss,
              onRecovery: recovery,
              loadCollection: {period: config.cluster.report_load_interval,
                               item: {name: 'cpu'}}
             };

  worker = require('./clusterWorker')(spec);
};

var serviceObserver = {
  onJoin: function(tokenCode) {
    worker && worker.addTask(tokenCode);
  },
  onLeave: function(tokenCode) {
    worker && worker.removeTask(tokenCode);
  }
};

var startServers = function(id) {
    var rpcReq = require('./rpcRequest')();

  portal = require('./portal')({
    tokenServer: 'ManagementApi',
    clusterName: config.cluster.grpc_host,
    selfRpcId: id,
    customized_controller: config.portal.customized_controller
  }, rpcReq);

  socketio_server = require('./socketIOServer')({
    port: config.portal.port,
    cors: config.portal.cors,
    ssl: config.portal.ssl,
    forceTlsv12: config.portal.force_tls_v12,
    keystorePath: config.portal.keystorePath,
    reconnectionTicketLifetime: config.portal.reconnection_ticket_lifetime,
    reconnectionTimeout: config.portal.reconnection_timeout,
    pingInterval: config.portal.ping_interval,
    pingTimeout: config.portal.ping_timeout
  }, portal, serviceObserver);

  // For handling gRPC notifications
  rpcReq.notificationHandler = {
    notify: function(participantId, event, data) {
      const notifyFail = (err) => {};
      socketio_server && socketio_server.notify(participantId, event, data)
        .catch(notifyFail);
    },
    broadcast: function(controller, excludeList, event, data) {
      socketio_server && socketio_server.broadcast(
        controller, excludeList, event, data);
    },
    drop: function(participantId) {
      socketio_server && socketio_server.drop(participantId);
    },
  };

  return socketio_server.start()
    .then(function() {
      log.info('start socket.io server ok.');
    })
    .catch(function(err) {
      log.error('Failed to start servers, reason:', err && err.message);
      throw err;
    });
};

var stopServers = function() {
  socketio_server && socketio_server.stop();
  socketio_server = undefined;
  worker && worker.quit();
  worker = undefined;
};

var rpcPublic = {
  drop: function(participantId, callback) {
    socketio_server && socketio_server.drop(participantId);
    callback('callback', 'ok');
  },
  notify: function(participantId, event, data, callback) {
    // The "notify" is called on socket.io server,
    // but one client ID should not be exists in both servers,
    // there must be one failure, ignore this notify error here.
    var notifyFail = (err) => {};
    socketio_server && socketio_server.notify(participantId, event, data).catch(notifyFail);
    callback('callback', 'ok');
  },
  broadcast: function(controller, excludeList, event, data, callback) {
    socketio_server && socketio_server.broadcast(controller, excludeList, event, data);
    callback('callback', 'ok');
  },
  onNotification: function(endpoints, name, data, callback) {
    if (endpoints.to) {
      socketio_server && socketio_server.notify(endpoints.to, name, data)
          .catch((err) => log.debug('notify err:', err));
    } else if (endpoints.domain) {
      socketio_server && socketio_server.broadcast(
          endpoints.domain, [endpoints.from], name, data);
    } else {
      log.info('Invalid endpoints:', endpoints);
    }
    callback('callback', 'ok');
  },
};

  // Run gRPC mode
  joinCluster(function(id) {
    startServers(id);
  });

['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, async function () {
    log.warn('Exiting on', sig);
    stopServers();
    process.exit();
  });
});

process.on('SIGPIPE', function () {
  log.warn('SIGPIPE!!');
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
