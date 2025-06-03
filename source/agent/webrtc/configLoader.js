'use strict';
var networkHelper = require('./networkHelper');
const dataAccess = require('./data_access');

module.exports.load = async function () {
  try {
    let webrtc_agent = await dataAccess.Config.findOne({
      name: 'webrtc_agent',
    });
    let config = Object.fromEntries(webrtc_agent.config.entries()) || {};

    config.agent = config.agent || {};
    config.agent.maxProcesses = config.agent.maxProcesses || 16;
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
      name: 'network',
      interf: config.cluster.network_name || 'lo',
      max_scale: config.cluster.network_max_scale || 1000,
    };

    config.capacity = config.capacity || {};
    config.capacity.isps = config.capacity.isps || [];
    config.capacity.regions = config.capacity.regions || [];

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

    config.webrtc = config.webrtc || {};
    config.webrtc.stunserver = config.webrtc.stunserver || '';
    config.webrtc.stunport = config.webrtc.stunport || 0;
    config.webrtc.minport = config.webrtc.minport || 0;
    config.webrtc.maxport = config.webrtc.maxport || 0;
    config.webrtc.keystorePath = config.webrtc.keystorePath || '';
    config.webrtc.num_workers = config.webrtc.num_workers || 24;
    config.webrtc.use_nicer = config.webrtc.use_nicer || false;
    config.webrtc.io_workers = config.webrtc.io_workers || 8;
    config.webrtc.network_interfaces = config.webrtc.network_interfaces || [];

    config.webrtc.network_interfaces.forEach((item) => {
      let addr = networkHelper.getAddress(item.name);
      if (!addr) {
        console.error("Can't get webrtc IP address");
        process.exit(1);
      }
      item.ip_address = addr.ip;

      // Parse webrtc ip_address variables from ENV.
      if (
        item.replaced_ip_address &&
        item.replaced_ip_address.indexOf('$') == 0
      ) {
        item.replaced_ip_address =
          process.env[item.replaced_ip_address.substr(1)];
        console.log(
          'ENV: config.webrtc.network_interfaces[' +
            item.name +
            '].replaced_ip_address=' +
            item.replaced_ip_address
        );
      }
    });

    return config;
  } catch (e) {
    console.error('ERROR on read config: ' + e.message);
    process.exit(1);
  }
};
