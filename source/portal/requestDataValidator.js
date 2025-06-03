/*
 * JSON Validator for Request Data
 */

const Ajv = require('ajv');
// Using json schema validate with 'defaults' and 'removeAdditional' properties
const ajv = new Ajv({ useDefaults: true });

const ReqType = require('./requestType');
const {
  PublicationRequest,
  StreamControlInfo,
  SubscriptionRequest,
  SubscriptionControlInfo,
} = require('./requestFormat');

function generateValidator(schema) {
  var rawValidate = ajv.compile(schema);
  var validate = function (instance) {
    if (rawValidate(instance)) {
      return Promise.resolve(instance);
    } else {
      return Promise.reject(getErrorMessage(rawValidate.errors));
    }
  };

  return validate;
}

function getErrorMessage(errors) {
  var error = errors.shift();
  var message = '';
  var messages = [];
  while (error) {
    if (error.keyword === 'anyOf') {
      error = errors.shift();
      continue;
    }
    var message = 'request' + error.dataPath + ' ' + error.message;
    for (let key in error.params) {
      let param = error.params[key];
      if (param) {
        message += ' ' + JSON.stringify(param);
      }
    }

    messages.push(message);
    error = errors.shift();
  }

  return messages.join(',');
}

const validateTextReq = (textReq) => {
  if (textReq.to === '' || typeof textReq.to !== 'string') {
    return Promise.reject('Invalid receiver');
  } else {
    return Promise.resolve(textReq);
  }
};

const validateSOAC = (SOAC) => {
  if (typeof SOAC.id !== 'string') {
    return Promise.reject('Invalid soac id');
  }
  if (!SOAC.signaling) {
    return Promise.reject('Invalid signaling object');
  }
  if (
    SOAC.signaling.type === 'offer' ||
    SOAC.signaling.type === 'answer' ||
    SOAC.signaling.type === 'candidate' ||
    SOAC.signaling.type === 'removed-candidates'
  ) {
    return Promise.resolve(SOAC);
  } else {
    return Promise.reject('Invalid signaling type');
  }
};

// Export JSON validator functions
module.exports = function () {
  let validators = {};
  validators[ReqType.Pub] = generateValidator(PublicationRequest);
  validators[ReqType.StreamCtrl] = generateValidator(StreamControlInfo);
  validators[ReqType.Sub] = generateValidator(SubscriptionRequest);
  validators[ReqType.SubscriptionCtrl] = generateValidator(
    SubscriptionControlInfo
  );
  validators[ReqType.Text] = validateTextReq;
  validators[ReqType.SOAC] = validateSOAC;

  return {
  validate: (spec, data) => {
    if (validators[spec]) {
      return validators[spec](data);
    } else {
      throw new Error(`No such validator: ${spec}`);
    }
  },
  };
};
