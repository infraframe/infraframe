'use strict';
var mongoose = require('mongoose');
var Schema = mongoose.Schema;

var ConfigSchema = new Schema({
  name: {
    type: String,
    required: true,
  },
  config: {
    type: Map,
    required: true,
  },
});

module.exports = mongoose.model('config', ConfigSchema);
