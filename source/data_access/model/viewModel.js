'use strict';
const mongoose = require('mongoose');
const Fraction = require('fraction.js');
const Schema = mongoose.Schema;
const DefaultUtil = require('../defaults');

const ColorRGB = {
  type: Number,
  min: 0,
  max: 255,
  default: 0,
};

const RNumber = {
  type: String,
  validate: {
    validator: function (v) {
      try {
        new Fraction(v);
      } catch (e) {
        return false;
      }
      return true;
    },
    message: '{VALUE} is not a valid fraction string!',
  },
  required: true,
};

const Region = {
  _id: false,
  id: { type: String, required: true },
  shape: { type: String, enum: ['rectangle'], default: 'rectangle' },
  area: {
    left: RNumber,
    top: RNumber,
    width: RNumber,
    height: RNumber,
  },
};

const Resolution = {
  width: { type: Number, default: 1920 },
  height: { type: Number, default: 1080 },
};

const AudioSchema = new Schema(
  {
    codec: { type: String },
    sampleRate: { type: Number },
    channelNum: { type: Number },
  },
  { _id: false }
);

const VideoSchema = new Schema(
  {
    codec: { type: String },
    profile: { type: String },
  },
  { _id: false }
);


var PositionTop = {
  x: { type: Number, default:100},
  y: { type: Number, default:100},
};

var PositionBottom = {
  x: { type: Number, default:200},
  y: { type: Number, default:200},
};

var DateTime = {
  enable: { type: Boolean, default: false },
  timeSync: { type: Boolean, default: false },
  content:{ type: String, default:''},
  format:{ type: String, default:'{:%Y-%m-%d %H:%M:%S}'},
  fontSize: { type: String, default:'32'},
  color: { type: String, default:'#ffffff'},
  leftTop: PositionTop,
  rightBottom: PositionBottom,
};

var Text = {
  enable: { type: Boolean, default: false },
  content:{ type: String, default:'text'},
  fontSize: { type: String, default:'32'},
  color: { type: String, default:'#ffffff'},
  leftTop: PositionTop,
  rightBottom: PositionBottom,
};

var Picture = {
  enable: { type: Boolean, default: false },
  imagePath: { type: String, default:''},
  transparency:{ type: Number, default:90},
  scale: { type: Number, default:1},
  rotation: { type: Number, default:0},
  leftTop: PositionTop,
  rightBottom: PositionBottom,
};

const ViewSchema = new Schema(
  {
    label: { type: String, default: 'common' },
    audio: {
      format: { type: AudioSchema, default: DefaultUtil.AUDIO_OUT[0] },
      vad: { type: Boolean, default: false },
    },
    video: {
      format: { type: VideoSchema, default: DefaultUtil.VIDEO_OUT[0] },
      parameters: {
        resolution: Resolution,
        framerate: { type: Number, default: 24 },
        bitrate: { type: Number },
        keyFrameInterval: { type: Number, default: 100 },
      },
      maxInput: { type: Number, default: 16, min: 1, max: 256 },
      motionFactor: { type: Number, default: 0.8 },
      bgColor: { r: ColorRGB, g: ColorRGB, b: ColorRGB },
      keepActiveInputPrimary: { type: Boolean, default: false },
      layout: {
        //TODO: stretched?
        fitPolicy: {
          type: String,
          enum: ['letterbox', 'crop'],
          default: 'letterbox',
        },
        setRegionEffect: { type: String },
        templates: {
          base: {
            type: String,
            enum: ['fluid', 'lecture', 'void'],
            default: 'fluid',
          },
          custom: [
            {
              _id: false,
              primary: { type: String },
              region: [Region],
            },
          ],
        },
      },
    },
    osd: {
      dateTime: DateTime,
      text: Text,
      picture: Picture,
    },
    storagePlan: {
      enable: { type: Boolean, default: false },
    }
  },
  { _id: false }
);

module.exports = mongoose.model('View', ViewSchema);
