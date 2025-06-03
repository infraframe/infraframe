// SPDX-License-Identifier: Apache-2.0

'use strict';

/*
 * common Util
 */

const clamp = (value,min, max) => {
  return Math.min(Math.max(value,min),max);
};

const isObjInArray = (obj,array) =>{
  for(let i = 0;i < array.length;i++){
    if(JSON.stringify(obj) === JSON.stringify(array[i])){
      return true;
    }
  }
  return false;
};


module.exports = {
  clamp,
  isObjInArray,
};
