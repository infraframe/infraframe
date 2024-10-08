#!/usr/bin/env node

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var dbURL = process.env.DB_URL;
if (!dbURL) {
  throw 'DB_URL not found';
}

var currentVersion = '1.0';
var fs = require('fs');
var path = require('path');

var MongoClient = require('mongodb').MongoClient;
var connectOption = {
  useUnifiedTopology: true,
};
var client;
var db;
var cipher = require('./cipher');

var CONFIG_NAME = 'management_api.toml';
var SAMPLE_RELATED_PATH = '../apps/current_app/';
var DEFAULT_SAMPLE_ENTRY = 'samplertcservice.js';

var dirName = !process.pkg ? __dirname : path.dirname(process.execPath);
var configFile = path.resolve(dirName, CONFIG_NAME);
var samplePackageJson = path.resolve(
  dirName,
  SAMPLE_RELATED_PATH,
  'package.json'
);
var sampleEntryName = require(samplePackageJson).main || DEFAULT_SAMPLE_ENTRY;
var sampleServiceFile = path.resolve(
  dirName,
  SAMPLE_RELATED_PATH,
  sampleEntryName
);
var spk = cipher.dk;

async function prepareDB(next) {
  if (dbURL.indexOf('mongodb://') !== 0) {
    dbURL = 'mongodb://' + dbURL;
  }
  if (fs.existsSync(cipher.astore)) {
    cipher.unlock(cipher.k, cipher.astore, function cb(err, authConfig) {
      if (!err) {
        if (authConfig.spk) {
          spk = Buffer.from(authConfig.spk, 'hex');
        }
        if (authConfig.mongo && !dbURL.includes('@')) {
          dbURL =
            'mongodb://' +
            authConfig.mongo.username +
            ':' +
            authConfig.mongo.password +
            '@' +
            dbURL.replace('mongodb://', '');
        }
      } else {
        console.error('Failed to get mongodb auth:', err);
      }
      MongoClient.connect(dbURL, connectOption, function (err, cli) {
        if (!err) {
          client = cli;
          db = client.db();
          next();
        } else {
          console.error('Failed to connect mongodb:', err);
        }
      });
    });
  } else {
    client = new MongoClient(dbURL);
    await client.connect();
    db = client.db('owtdb');
    next();
  }
}

async function upgradeH264(next) {
  let rooms = await db.collection('rooms').find({}).toArray();

  if (!rooms || rooms.length === 0) {
    next();
    return;
  }

  var total = rooms.length;
  var count = 0;
  var i, room;
  for (i = 0; i < total; i++) {
    room = rooms[i];
    room.mediaOut.video.format.forEach((fmt) => {
      if (fmt && fmt.codec === 'h264' && !fmt.profile) {
        fmt.profile = 'CB';
      }
    });
    room.mediaOut.video.format = room.mediaOut.video.format.filter((fmt) => {
      return fmt && fmt.codec !== 'h265';
    });
    room.mediaIn.video = room.mediaIn.video.filter((fmt) => {
      return fmt && fmt.codec !== 'h265';
    });
    room.views.forEach((viewSettings) => {
      var fmt = viewSettings.video.format;
      if (fmt && fmt.codec === 'h264' && !fmt.profile) {
        fmt.profile = 'CB';
      } else if (fmt && fmt.codec === 'h265') {
        fmt.codec = 'h264';
        fmt.profile = 'CB';
      } else if (!fmt) {
        viewSettings.video.format = { codec: 'vp8' };
      }
    });

    let saved = await db.collection('rooms').updateOne({ _id: room._id }, room);
    count++;
    if (count === total) {
      next();
    }
  }
}

async function checkVersion(next) {
  let info = await db.collection('infos').findOne({ _id: 1 });
  if (info) {
    if (info.version === '1.0') {
      next();
    }
  } else {
    let service = await db.collection('services').findOne({});
    var upgradeNext = function (next) {
      upgradeH264(function () {
        info = { _id: 1, version: currentVersion };
        db.collection('infos').insertOne(info);
        next();
      });
    };
    if (service) {
      if (typeof service.__v !== 'number') {
        console.log(
          `The existed service "${service.name}" is not in 4.* format.`
        );
        console.log('Preparing to upgrade your database.');
        require('./SchemaUpdate3to4').update(function () {
          upgradeNext(next);
        });
      } else {
        var rl = require('readline').createInterface({
          input: process.stdin,
          output: process.stdout,
        });
        rl.question(
          'This operation will upgrade stored data to version 4.1. Are you ' +
            'sure you want to proceed this operation anyway?[y/n]',
          (answer) => {
            rl.close();
            answer = answer.toLowerCase();
            if (answer === 'y' || answer === 'yes') {
              upgradeNext(next);
            } else {
              process.exit(0);
            }
          }
        );
      }
    } else {
      upgradeNext(next);
    }
  }
}

async function prepareService(serviceName, next) {
  let service = await db.collection('services').findOne({ name: serviceName });
  if (!service) {
    var crypto = require('crypto');
    var key = crypto
      .pbkdf2Sync(
        crypto.randomBytes(64).toString('hex'),
        crypto.randomBytes(32).toString('hex'),
        4000,
        128,
        'sha256'
      )
      .toString('base64');
    service = {
      name: serviceName,
      key: cipher.encrypt(spk, key),
      encrypted: true,
      rooms: [],
      __v: 0,
    };
    let result = await db.collection('services').insertOne(service);
    result.key = key;
    next(result);
  } else {
    if (service.encrypted === true) {
      service.key = cipher.decrypt(spk, service.key);
    }
    next(service);
  }
}

async function writeConfigFile(superServiceId, superServiceKey) {
  try {
    fs.statSync(configFile);
    fs.readFile(configFile, 'utf8', function (err, data) {
      if (err) {
        return console.log(err);
      }
      data = data.replace(
        /\ndataBaseURL =[^\n]*\n/,
        '\ndataBaseURL = "' + dbURL + '"\n'
      );
      data = data.replace(
        /\nsuperserviceID =[^\n]*\n/,
        '\nsuperserviceID = "' + superServiceId + '"\n'
      );
      fs.writeFile(configFile, data, 'utf8', function (err) {
        if (err) return console.log('Error in saving configuration:', err);
      });
    });
  } catch (e) {
    console.error('config file not found:', configFile);
  }
}

async function writeSampleFile(sampleServiceId, sampleServiceKey) {
  fs.readFile(sampleServiceFile, 'utf8', function (err, data) {
    if (err) {
      return console.log(err);
    }
    data = data.replace(
      /icsREST\.API\.init\('[^']*', '[^']*'/,
      "icsREST.API.init('" + sampleServiceId + "', '" + sampleServiceKey + "'"
    );
    fs.writeFile(sampleServiceFile, data, 'utf8', function (err) {
      if (err) return console.log(err);
    });
  });
}

prepareDB(function () {
  checkVersion(function () {
    prepareService('superService', function (service) {
      var superServiceId = service._id.toString();
      var superServiceKey = service.key;
      console.log('superServiceId:', superServiceId);
      console.log('superServiceKey:', superServiceKey);
      writeConfigFile(superServiceId, superServiceKey);

      prepareService('sampleService', function (service) {
        var sampleServiceId = service._id.toString();
        var sampleServiceKey = service.key;
        console.log('sampleServiceId:', sampleServiceId);
        console.log('sampleServiceKey:', sampleServiceKey);
        client.close();

        writeSampleFile(sampleServiceId, sampleServiceKey);
      });
    });
  });
});
