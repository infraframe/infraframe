'use strict';
const dataAccess = require('./data_access');
const mongoose = require('mongoose');
const Default = require('./data_access/defaults');

/* 各模块/agent的配置项 */
async function initConfig() {
  const cluster_name = 'owt-cluster';

  // global
  const global = {
    name: 'global',
    config: {
    },
  };
  await dataAccess.Config.create(global);

  // analytics_agent
  const config_analytics_agent = {
    name: 'analytics_agent',
    config: {
      agent: {
        maxProcesses: 10,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
        load_items: 'CPU',
      },
      capacity: {
        isps: [],
        regions: [],
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      analytics: {
        libpath: '',
        hardwareAccelerated: true,
        HDDL: false,
      },
    },
  };
  await dataAccess.Config.create(config_analytics_agent);

  // audio_agent
  const config_audio_agent = {
    name: 'audio_agent',
    config: {
      agent: {
        maxProcesses: -1,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
      },
      capacity: {
        isps: [],
        regions: [],
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      mix: {
        top_k: 0,
      },
    },
  };
  await dataAccess.Config.create(config_audio_agent);

  // cluster_manager
  const config_cluster_manager = {
    name: 'cluster_manager',
    config: {
      agent: {
        maxProcesses: -1,
        prerunProcessed: 2,
      },
      manager: {
        name: cluster_name,
        initial_time: 6000,
        check_alive_interval: 1000,
        check_alive_count: 3,
        schedule_reserve_time: 60000,
        grpc_host: 'localhost:10080',
      },
      strategy: {
        general: 'last-used',
        analytics: 'least-used',
        portal: 'last-used',
        conference: 'last-used',
        webrtc: 'last-used',
        streaming: 'round-robin',
        recording: 'randomly-pick',
        audio: 'most-used',
        video: 'least-used',
      },
    },
  };
  await dataAccess.Config.create(config_cluster_manager);

  // conference_agent
  const config_conference_agent = {
    name: 'conference_agent',
    config: {
      agent: {
        maxProcesses: -1,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
      },
      internal: {
        protocol: 'tcp',
      },
      mongo: {
        dataBaseURL: 'localhost/owtdb',
      },
    },
  };
  await dataAccess.Config.create(config_conference_agent);


  // management_api
  const config_management_api = {
    name: 'management_api',
    config: {
      server: {
        keystorePath: './cert/certificate.pfx',
        ssl: true,
        port: 3000,
        numberOfProcess: 4,
        hostname: '',
        ip_address: '',
      },
      mongo: {
        dataBaseURL: 'localhost/owtdb',
      },
    },
  };
  await dataAccess.Config.create(config_management_api);

  // portal
  const config_portal = {
    name: 'portal',
    config: {
      portal: {
        keystorePath: './cert/certificate.pfx',
        hostname: '',
        ip_address: '',
        port: 8080,
        ssl: true,
        force_tls_v12: false,
        networkInterface: 'eth1',
        ping_interval: 25,
        ping_timeout: 60,
        reconnection_ticket_lifetime: 600,
        reconnection_timeout: 60,
        cors: ['*'],
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
      },
      capacity: {
        isps: [],
        regions: [],
      },
      mongo: {
        dataBaseURL: 'localhost/owtdb',
      },
    },
  };
  await dataAccess.Config.create(config_portal);

  // recording_agent
  const config_recording_agent = {
    name: 'recording_agent',
    config: {
      agent: {
        maxProcesses: 50,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      recording: {
        initialize_timeout: 3000,
      },
    },
  };
  await dataAccess.Config.create(config_recording_agent);

  // streaming_agent
  const config_streaming_agent = {
    name: 'streaming_agent',
    config: {
      agent: {
        maxProcesses: 10,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
        network_max_scale: 1000,
      },
      capacity: {
        isps: [],
        regions: [],
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      avstream: {
        initialize_timeout: 3000,
      },
    },
  };
  await dataAccess.Config.create(config_streaming_agent);

  // video_agent
  const config_video_agent = {
    name: 'video_agent',
    config: {
      agent: {
        maxProcesses: -1,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
      },
      capacity: {
        isps: [],
        regions: [],
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      video: {
        hardwareAccelerated: true,
        enableBetterHEVCQuality: false,
        MFE_timeout: 0,
      },
      // avatar: {
      //   location: 'avatars/avatar_blue.180x180.yuv',
      // },
    },
  };
  await dataAccess.Config.create(config_video_agent);

  // webrtc_agent
  const config_webrtc_agent = {
    name: 'webrtc_agent',
    config: {
      agent: {
        maxProcesses: 10,
        prerunProcessed: 2,
      },
      cluster: {
        name: cluster_name,
        join_retry: 60,
        report_load_interval: 1000,
        max_load: 0.85,
        network_max_scale: 1000,
        network_name: 'lo',
      },
      capacity: {
        isps: [],
        regions: [],
      },
      internal: {
        ip_address: '',
        maxport: 0,
        minport: 0,
      },
      webrtc: {
        keystorePath: './cert/certificate.pfx',
        network_interfaces: [],
        maxport: 0,
        minport: 0,
        stunport: 0,
        stunserver: '',
        num_workers: 24,
      },
    },
  };
  await dataAccess.Config.create(config_webrtc_agent);
}

/* 用户 */
async function initUser() {}

/* 房间 */
async function initRoom() {
  let room = {
    name: 'sampleRoom',
    inputLimit: 100,
    participantLimit: -1,
    selectActiveAudio: false,
    enableBandwidthAdaptation: false,
    transcoding: {
      audio: false,
      video: {
        format: true,
        parameters: {
          resolution: true,
          framerate: true,
          bitrate: true,
          keyFrameInterval: true,
        },
      },
    },
    notifying: {
      participantActivities: true,
      streamChange: true,
    },
    ...Default.ROOM_CONFIG,
  };
  await dataAccess.Room.create(room);
}

async function main() {
  await initConfig();
  await initRoom();
  await initUser();
}

main().finally(() => {
  mongoose.disconnect();
});
