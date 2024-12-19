// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('ManagementServer');
var fs = require('fs');

try {
  global.config = {
    server: {
      keystorePath: './cert/certificate.pfx',
      ssl: true,
      port: 3000,
      numberOfProcess: 4,
      hostname: '',
      ip_address: '',
    },
    mongo: {
      dataBaseURL: 'localhost/owtdb',
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

var e = require('./errors');

var express = require('express');
var bodyParser = require('body-parser');
var app = express();

var serverAuthenticator = require('./auth/serverAuthenticator');
var cipher = require('./cipher');

// parse application/x-www-form-urlencoded
app.use(bodyParser.urlencoded({ extended: true }));
// parse application/json
app.use(bodyParser.json());

// for CORS
app.use(function (req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header(
    'Access-Control-Allow-Methods',
    'POST, GET, OPTIONS, DELETE, PATCH'
  );
  res.header(
    'Access-Control-Allow-Headers',
    'origin, authorization, content-type'
  );
  res.header('Strict-Transport-Security', 'max-age=1024000; includeSubDomain');
  res.header('X-Content-Type-Options', 'nosniff');
  next();
});
app.options('*', function (req, res) {
  res.send(200);
});

// Only following paths need authentication.
var authPaths = ['/v1/rooms*', '/services*'];
app.get(authPaths, serverAuthenticator.authenticate);
app.post(authPaths, serverAuthenticator.authenticate);
app.delete(authPaths, serverAuthenticator.authenticate);
app.put(authPaths, serverAuthenticator.authenticate);
app.patch(authPaths, serverAuthenticator.authenticate);

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
// Mutiple process setup
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

  // Save server key to database.
  // FIXME: we should check if server key already exists
  // in cache/database before generating a new one when
  // there are multiple machine running server.
  // var dataAccess = require("./data_access");
  // dataAccess.token.genKey();

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
  // Load spk
  try {
    const aconfig = cipher.unlockSync(cipher.k, cipher.astore);
    global.config.spk = aconfig.spk
      ? Buffer.from(aconfig.spk, 'hex')
      : cipher.dk;
  } catch (e) {
    global.config.spk = cipher.dk;
  }

  if (serverConfig.ssl === true) {
    var cipher = require('./cipher');
    var path = require('path');
    var keystore = path.resolve(
      path.dirname(serverConfig.keystorePath),
      cipher.kstore
    );
    cipher.unlock(cipher.k, keystore, function cb(err, passphrase) {
      if (!err) {
        try {
          require('https')
            .createServer(
              {
                pfx: require('fs').readFileSync(serverConfig.keystorePath),
                passphrase: passphrase,
              },
              app
            )
            .listen(serverPort);
        } catch (e) {
          log.warn('Failed to start secured server:', e);
          return process.exit(1);
        }
      } else {
        log.warn('Failed to setup secured server:', err);
        return process.exit(1);
      }
    });
  } else {
    app.listen(serverPort);
  }

  log.info(`Worker ${process.pid} started`);

  ['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
      // This log won't be showed in server log,
      // because worker process disconnected.
      log.warn('Worker exiting on', sig);
      process.exit();
    });
  });

  process.on('exit', function () {
  });

  process.on('SIGUSR2', function () {
    logger.reconfigure();
  });
}
