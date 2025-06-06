'use strict';
const fs = require('fs');
const mongoose = require('mongoose');
const { exit } = require('process');

if (!process.env.MONGODB_DATABASE_URL) {
  console.error('未设置MONGODB_DATABASE_URL!');
  exit();
}
let databaseUrl = process.env.MONGODB_DATABASE_URL;

mongoose.plugin((schema) => {
  schema.options.usePushEach = true;
});
mongoose.Promise = Promise;

// Connect to MongoDB
let connectOption = {};

const setupConnection = function () {
  if (databaseUrl.indexOf('mongodb://') !== 0) {
    databaseUrl = 'mongodb://' + databaseUrl;
    }
  mongoose.set('strictQuery', false);
  mongoose.connect(databaseUrl, connectOption).catch(function (err) {
    console.log(err.message);
  });
  mongoose.connection.on('error', function (err) {
    console.log(err.message);
  });
};

  setupConnection();

exports.room = require('./interface/roomInterface');

exports.Config = require('./model/configModel');
exports.Room = require('./model/roomModel');
exports.View = require('./model/viewModel');
