'use strict';
/*
 * Request Data Format
 */

const Resolution = {
  id: '/Resolution',
  type: 'object',
  properties: {
    'width': { type: 'number' },
    'height': { type: 'number' }
  },
  additionalProperties: false,
  required: ['width', 'height']
};


// PublcationRequest
const PublicationRequest = {
  anyOf: [
    { // Webrtc Publication
      type: 'object',
      properties: {
        'media': { $ref: '#/definitions/WebRTCMediaOptions' },
        'attributes': { type: 'object' },
        'data': {type: 'boolean'},
        'transport': {
          type: 'object',
          properties: {
            'type': {type: 'string'},
            'id': {type: 'string'},
          },
          additionalProperties: false,
        },
      },
      additionalProperties: false,
      required: ['media']
    }
  ],

  definitions: {
    'WebRTCMediaOptions': {
      type: ['object', 'null'],
      properties: {
        'tracks': {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              'type': { enum: ["audio", "video"] },
              'mid': { type: 'string' },
              'source': { enum: ["mic", "camera", "screen-cast", "raw-file", "encoded-file"] },
            }
          }
        }
      },
      additionalProperties: false,
      required: ['tracks']
    }
  }
};


// StreamControlInfo
const StreamControlInfo = {
  type: 'object',
  anyOf: [
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['mix', 'unmix', 'get-region'] },
        'data': { type: 'string' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['set-region'] },
        'data': { $ref: '#/definitions/RegionSetting' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      },
      additionalProperties: false
    },
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'RegionSetting': {
      type: 'object',
      properties: {
        'view': { type: 'string' },
        'region': { type: 'string',  minLength: 1}
      },
      additionalProperties: false,
      required: ['view', 'region']
    }
  }
};


// SubscriptionRequest
const SubscriptionRequest = {
  anyOf: [
    { // Webrtc Subscription
      type: 'object',
      properties: {
        'media': { $ref: '#/definitions/MediaSubOptions' },
        'transport': {
          type: 'object',
          properties: {
            'type': {type: 'string'},
            'id': {type: 'string'},
          },
          additionalProperties: false,
        },
        'data': { $ref: '#/definitions/DataSubOptions' },
      },
      additionalProperties: false,
      required: ['media']
    }
  ],

  definitions: {
    'MediaSubOptions': {
      type: ['object', 'null'],
      properties: {
        'tracks': {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              'type': { enum: ["audio", "video"] },
              'mid': { type: 'string' },
              'from': { type: 'string' },
              'format': {
                anyOf: [
                  { $ref: '#/definitions/AudioFormat' },
                  { $ref: '#/definitions/VideoFormat' },
                ]
              },
              'parameters': { $ref: '#/definitions/VideoParametersSpecification' },
            }
          }
        }
      },
      additionalProperties: false,
      required: ['tracks']
    },

    'DataSubOptions': {
      type: 'object',
      properties: {
        'from': {type: 'string'}
      },
      additionalProperties: false,
      required: ['from']
    },

    'AudioFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['pcmu', 'pcma', 'opus', 'g722', 'iSAC', 'iLBC', 'aac', 'ac3', 'nellymoser'] },
        'sampleRate': { type: 'number' },
        'channelNum': { type: 'number' }
      },
      additionalProperties: false,
      required: ['codec']
    },

    'VideoFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['h264', 'h265', 'vp8', 'vp9'] },
        'profile': { enum: ['CB', 'B', 'M', 'E', 'H'] }
      },
      additionalProperties: false,
      required: ['codec']
    },

    'VideoParametersSpecification': {
      type: 'object',
      properties: {
        'resolution': Resolution,
        'framerate': { type: 'number' },
        'bitrate': { type: ['string', 'number'] },
        'keyFrameInterval': { type: 'number' }
      },
      additionalProperties: false
    }
  }
};


// SubscriptionControlInfo
const SubscriptionControlInfo = {
  type: 'object',
  anyOf: [
    {
      properties: {
        'id': { type: 'string' },
        'operation': { enum: ['update'] },
        'data': { $ref: '#/definitions/SubscriptionUpdate' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string' },
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      },
      additionalProperties: false
    }
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'SubscriptionUpdate': {
      type: 'object',
      properties: {
        'audio': { $ref: '#/definitions/AudioUpdate' },
        'video': { $ref: '#/definitions/VideoUpdate' }
      },
      additionalProperties: false
    },

    'AudioUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'VideoUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'parameters': { $ref: '#/definitions/VideoUpdateSpecification' }
      },
      additionalProperties: false
    },

    'VideoUpdateSpecification': {
      type: 'object',
      properties: {
        resolution: Resolution,
        framerate: { type: 'number' },
        bitrate: { type: ['number', 'string'] },
        keyFrameInterval: { type: 'number' }
      },
      additionalProperties: false
    }
  }
};

const TransportOptions = {
  type: 'object',
  properties: {
    'type': { enum: ["webrtc", "quic"] },
    'id': { type: 'string' }
  }
};

module.exports = {
  PublicationRequest,
  StreamControlInfo,
  SubscriptionRequest,
  SubscriptionControlInfo,
};
