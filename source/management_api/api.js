'use strict';
const logger = require('./logger').logger;
const log = logger.getLogger('ManagementServer');
const e = require('./errors');
const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const serverAuthenticator = require('./auth/serverAuthenticator');
const path = require('path');
const dataAccess = require('./data_access');

async function main() {
  try {
    let management_api = await dataAccess.Config.findOne({
      name: 'management_api',
    });
    global.config = Object.fromEntries(management_api.config.entries()) || {};
    console.log(global.config);
  } catch (e) {
    log.error('ERROR on read config: ' + e.message);
    process.exit(1);
  }

  // parse application/x-www-form-urlencoded
  app.use(bodyParser.urlencoded({ extended: true }));
  // parse application/json
  app.use(bodyParser.json());

  // for CORS
  app.use(function (req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header(
      'Access-Control-Allow-Methods',
      'POST, GET, OPTIONS, DELETE, PATCH ,PUT'
    );
    res.header(
      'Access-Control-Allow-Headers',
      'origin, authorization, content-type,Accept,X-Requested-With,Cache-Control,Pragma'
    );
    res.header(
      'Strict-Transport-Security',
      'max-age=1024000; includeSubDomain'
    );
    res.header('X-Content-Type-Options', 'nosniff');
    next();
  });
  app.options('*', function (req, res) {
    res.sendStatus(200);
  });
  // API for version 1.0.
  var routerV1 = require('./resource/v1');
  app.use('/v1', routerV1);

  // for path not match
  app.use('*', function (req, res, next) {
    next(new e.NotFoundError());
  });

  // error handling
  app.use(function (err, req, res, next) {
    log.debug(err);
    if (!(err instanceof e.AppError)) {
      if (err instanceof SyntaxError) {
        err = new e.BadRequestError('Failed to parse JSON body');
      } else {
        log.warn(err, err.stack);
        err = new e.AppError(err.name + ' ' + err.message);
      }
    }
    res.status(err.status).send(err.data);
  });

  var serverConfig = global.config.server || {};
  // Multiple process setup
  var cluster = require('cluster');
  var serverPort = serverConfig.port || 3000;
  var numCPUs = serverConfig.numberOfProcess || 1;

  var ip_address;
  (function getPublicIP() {
    var BINDED_INTERFACE_NAME = serverConfig.networkInterface;
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

    if (serverConfig.hostname === '' || serverConfig.hostname === undefined) {
      if (
        serverConfig.ip_address === '' ||
        serverConfig.ip_address === undefined
      ) {
        ip_address = addresses[0];
      } else {
        ip_address = serverConfig.ip_address;
      }
    } else {
      ip_address = serverConfig.hostname;
    }
  })();

  var url;
  if (serverConfig.ssl === false) {
    url = 'http://' + ip_address + ':' + serverPort;
  } else {
    url = 'https://' + ip_address + ':' + serverPort;
  }

  if (cluster.isMaster) {
    // Master Process

    for (var i = 0; i < numCPUs; i++) {
      cluster.fork();
    }

    cluster.on('exit', function (worker, code, signal) {
      log.info(`Worker ${worker.process.pid} died`);
    });

    ['SIGINT', 'SIGTERM'].map(function (sig) {
      process.on(sig, function () {
        log.info('Master exiting on', sig);
        process.exit();
      });
    });

    process.on('SIGUSR2', function () {
      logger.reconfigure();
    });
  } else {
    app.listen(serverPort);
    log.info(`Worker ${process.pid} started`);

    ['SIGINT', 'SIGTERM'].map(function (sig) {
      process.on(sig, async function () {
        // This log won't be showed in server log,
        // because worker process disconnected.
        log.warn('Worker exiting on', sig);
        process.exit();
      });
    });

    process.on('exit', function () {});

    process.on('SIGUSR2', function () {
      logger.reconfigure();
    });
  }
}

main().catch((err) => console.log(err));
