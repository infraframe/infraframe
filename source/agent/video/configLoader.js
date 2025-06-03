'use strict';
var fs = require('fs');
var networkHelper = require('./networkHelper');
const dataAccess = require('./data_access');

module.exports.load = async function () {
  try {
    let video_agent = await dataAccess.Config.findOne({ name: 'video_agent' });
    let config = Object.fromEntries(video_agent.config.entries()) || {};

    config.agent = config.agent || {};
    config.agent.maxProcesses = config.agent.maxProcesses || -1;
    config.agent.prerunProcesses = config.agent.prerunProcesses || 2;

    config.cluster = config.cluster || {};
    config.cluster.name = config.cluster.name || 'owt-cluster';
    config.cluster.worker = config.cluster.worker || {};
    config.cluster.worker.ip =
      config.cluster.worker.ip ||
      (networkHelper.getAddress('firstEnumerated') || {}).ip ||
      'unknown';
    config.cluster.worker.join_retry = config.cluster.worker.join_retry || 60;
    config.cluster.worker.load = config.cluster.worker.load || {};
    config.cluster.worker.load.max = config.cluster.max_load || 0.85;
    config.cluster.worker.load.period =
      config.cluster.report_load_interval || 1000;
    config.cluster.worker.load.item = {
      name: 'cpu',
    };

    config.internal.ip_address = config.internal.ip_address || '';
    config.internal.network_interface =
      config.internal.network_interface || undefined;
    config.internal.minport = config.internal.minport || 0;
    config.internal.maxport = config.internal.maxport || 0;

    if (!config.internal.ip_address) {
      let addr = networkHelper.getAddress(
        config.internal.network_interface || 'firstEnumerated'
      );
      if (!addr) {
        console.error("Can't get internal IP address");
        process.exit(1);
      }

      config.internal.ip_address = addr.ip;
    }

    config.video = config.video || {};
    config.video.hardwareAccelerated = !!config.video.hardwareAccelerated;
    config.video.enableBetterHEVCQuality =
      !!config.video.enableBetterHEVCQuality;
    config.video.MFE_timeout = config.video.MFE_timeout || 0;
    let videoCap = require('./videoCapability').detected(
      config.video.hardwareAccelerated
    );
    config.video.hardwareAccelerated = videoCap.hw;
    config.video.codecs = videoCap.codecs;

    if (videoCap.hw) {
      config.cluster.worker.load.item.name = 'gpu';
    }

    config.capacity = config.capacity || {};
    config.capacity.video = videoCap.codecs;

    config.avatar = config.avatar || {};
    config.avatar.location =
      config.avatar.location || 'avatars/avatar_blue.180x180.yuv';

    return config;
  } catch (e) {
    console.error('ERROR on read config: ' + e.message);
    process.exit(1);
  }
};
