'use strict';

var crypto = require('crypto');
var requestHandler = require('../../requestHandler');
var logger = require('../../logger').logger;
const axios = require('axios');

// Logger
var log = logger.getLogger('TokensResource');

/*
 * Post Token. Creates a new token.
 */
exports.create = function (req, res, next) {
  let userCenterUrl = process.env.USER_CENTER_URL;
  if (!userCenterUrl) {
    return res
      .status(500)
      .json({ message: 'USER_CENTER_URL environment variable is not set' });
  }

  let origin = (req.body && req.body.preference) || {
    isp: 'isp',
    region: 'region',
  };
  let currentRoom = req.params.room;
  let username = req.body.user;
  let password = req.body.password;
  let token = {};

  axios({
    method: 'post',
    url: `${userCenterUrl}/admin-api/system/auth/login`,
    headers: {
      'accept': '*/*',
      'Content-Type': 'application/json',
      'tenant-id': 1,
      'Authorization': 'Basic ZGVmYXVsdDphZG1pbjEyMw==',
    },
    data: {
      username,
      password,
    },
  })
    .then(function (tokenData) {
      //   let tokenData = {
      //     code: 0,
      //     data: {
      //       userId: 184,
      //       accessToken: '4232eae844e74cf5a62848b05f0925d0',
      //       refreshToken: '7d84789ab90c43639498bc86bd71c99c',
      //       expiresTime: 1773978620877,
      //     },
      //     msg: '',
      //   };
      let data = tokenData.data.data;
      token.userId = data.userId;
      token.accessToken = data.accessToken;
      token.refreshToken = data.refreshToken;
      token.expiresTime = data.expiresTime;
      token.code = Math.floor(Math.random() * 100000000000) + '';
      token.origin = origin;
      token.room = currentRoom;
      axios({
        method: 'get',
        url: `${userCenterUrl}/admin-api/system/user/profile/get`,
        headers: {
          'tenant-id': 1,
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${data.accessToken}`,
        },
      })
        .then(function (userData) {
          let data = userData.data.data;
          //   let userData = {
          //     code: 0,
          //     data: {
          //       username: 'user',
          //       nickname: 'user',
          //       remark: null,
          //       deptId: null,
          //       postIds: [],
          //       email: null,
          //       mobile: null,
          //       sex: null,
          //       avatar: null,
          //       id: 184,
          //       status: 0,
          //       loginIp: '172.20.0.1',
          //       loginDate: 1742874619853,
          //       createTime: 1742874116601,
          //       roles: [{ id: 31, name: 'presenter' }],
          //       dept: null,
          //       posts: null,
          //       socialUsers: [],
          //     },
          //     msg: '',
          //   };
          requestHandler.schedulePortal(
            token.code,
            token.origin,
            function (ec) {
              if (ec === 'timeout') {
                callback('error');
                return;
              }

              if (ec.via_host && ec.via_host !== '') {
                if (ec.via_host.indexOf('https') == 0) {
                  token.secure = true;
                  token.host = ec.via_host.substr(8);
                } else {
                  token.secure = false;
                  token.host = ec.via_host.substr(7);
                }
              } else {
                token.secure = ec.ssl;
                if (ec.hostname !== '') {
                  token.host = ec.hostname;
                } else {
                  token.host = ec.ip;
                }

                token.host += ':' + ec.port;
              }

              token.username = data.username;
              token.role = data.roles[0].name;
              let tokenS = Buffer.from(JSON.stringify(token)).toString(
                'base64'
              );
              res.send(tokenS);
            }
          );
        })
        .catch(function (error) {
          res.status(403).json(error);
        });
    })
    .catch(function (error) {
      res.status(403).json(error);
    });
};
