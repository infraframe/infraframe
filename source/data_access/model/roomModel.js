'use strict';
const mongoose = require('mongoose');
const Schema = mongoose.Schema;
const Layout = require('./layoutTemplate');
const ViewModel = require('./viewModel');
const DefaultUtil = require('../defaults');

var RoomSchema = new Schema({
  name: {
    type: String,
    required: true,
  },
  inputLimit: {
    type: Number,
    default: -1,
  },
  participantLimit: {
    type: Number,
    default: -1,
  },
  selectActiveAudio: {
    type: Boolean,
    default: false,
  },
  enableBandwidthAdaptation: {
    type: Boolean,
    default: false,
  },
  roles: [],
  views: [ViewModel.schema],
  mediaIn: {
    audio: [],
    video: [],
  },
  mediaOut: {
    audio: [],
    video: {
      format: [],
      parameters: {
        resolution: [],
        framerate: [],
        bitrate: [],
        keyFrameInterval: [],
      },
    },
  },
  transcoding: {
    audio: { type: Boolean, default: true },
    video: {
      format: { type: Boolean, default: true },
      parameters: {
        resolution: { type: Boolean, default: true },
        framerate: { type: Boolean, default: true },
        bitrate: { type: Boolean, default: true },
        keyFrameInterval: { type: Boolean, default: true },
      },
    },
  },
  sip: {
    sipServer: String,
    username: String,
    password: {
      type: String,
      set: (v) => {
        return cipher.encrypt(cipher.k, v);
      },
      get: (v) => {
        if (!v) return v;
        let ret = '';
        try {
          ret = cipher.decrypt(cipher.k, v);
        } catch (e) {}
        return ret;
      },
    },
  },
  notifying: {
    participantActivities: { type: Boolean, default: true },
    streamChange: { type: Boolean, default: true },
  },
});

RoomSchema.set('toObject', { getters: true });

RoomSchema.statics.processLayout = function (room) {
  if (room && room.views) {
    room.views.forEach(function (view) {
      if (view.video) {
        view.video.layout.templates = Layout.applyTemplate(
          view.video.layout.templates.base,
          view.video.maxInput,
          view.video.layout.templates.custom
        );
      }
    });
  }
  return room;
};

module.exports = mongoose.model('Room', RoomSchema);
