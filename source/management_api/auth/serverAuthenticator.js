'use strict';
const log = require('./../logger').logger.getLogger('ServerAuthenticator');
const e = require('../errors');
const _ = require('lodash');

/*
 * This function has the logic needed for authenticate a request.
 * If the authentication success exports the user and role (if needed). Else send back
 * a response with an authentication request to the client.
 */
exports.authenticate = function (req, res, next) {
  let userCenterUrl = process.env.USER_CENTER_URL;
  if (!userCenterUrl) {
    return res
      .status(500)
      .json({ message: 'USER_CENTER_URL environment variable is not set' });
  }

  // 检查路径是否匹配 /v1/rooms/:room/tokens
  if (req.path.match(/^\/v1\/rooms\/[^\/]+\/tokens$/)) {
    return next(); // 跳过认证
  }

  // 获取 Authorization 头
  const authHeader =
    req.headers['authorization'] || req.headers['Authorization'];
  // 检查是否存在 Authorization 头
  if (!authHeader) {
    return res.status(401).json({ message: 'Authorization header is missing' });
  }
  // 检查 Authorization 头的格式是否为 Bearer Token
  const [bearer, token] = authHeader.split(' ');
  if (bearer !== 'Bearer' || !token) {
    return res
      .status(401)
      .json({ message: 'Invalid Authorization header format' });
        }

  axios({
    method: 'post',
    url: `${userCenterUrl}/admin-api/system/oauth2/check-token?token=${token}`,
    headers: {
      'tenant-id': 1,
      Authorization: 'Basic ZGVmYXVsdDphZG1pbjEyMw==',
    },
  })
    .then(function (tokenData) {
        next();
    })
    .catch(function (error) {
      res.status(401).json({ message: 'Invalid token' });
    });
};
