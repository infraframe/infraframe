#!/bin/bash

ssh-add ./infraframe_ci_git_key

if [ -d "infraframe" ]; then
    cd infraframe
    git pull
    cd ..
else
    git clone git@github.com:infraframe/infraframe.git
fi

if [ -d "infraframe-DEPS" ]; then
    cd infraframe-DEPS
    git pull
    cd ..
else
    git clone git@github.com:infraframe/infraframe-deps.git
fi

if [ -d "infraframe-js" ]; then
    cd infraframe-js
    git pull
    cd ..
else
    git clone git@github.com:infraframe/infraframe-js.git
fi

# build --platform linux/amd64,linux/arm64 the Docker images for each target
docker build --platform linux/amd64,linux/arm64 -f Dockerfile.builder -t infraframe/builder:latest .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/builder:latest"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.base -t infraframe/base:latest .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/base:latest"
  exit 1
fi

# docker build --platform linux/amd64,linux/arm64-f Dockerfile.analytics_agent -t infraframe/analytics_agent:1.0 .
docker build --platform linux/amd64,linux/arm64 -f Dockerfile.audio_agent -t infraframe/audio_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/audio_agent:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.cluster_manager -t infraframe/cluster_manager:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/cluster_manager:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.conference_agent -t infraframe/conference_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/conference_agent:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.management_api -t infraframe/management_api:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/management_api:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.portal -t infraframe/portal:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/portal:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.recording_agent -t infraframe/recording_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/recording_agent:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.streaming_agent -t infraframe/streaming_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/streaming_agent:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.video_agent -t infraframe/video_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/video_agent:1.0"
  exit 1
fi

docker build --platform linux/amd64,linux/arm64 -f Dockerfile.webrtc_agent -t infraframe/webrtc_agent:1.0 .
if [ $? -ne 0 ]; then
  echo "Error: Failed to build infraframe/webrtc_agent:1.0"
  exit 1
fi

# app
# docker build --platform linux/amd64,linux/arm64 -f Dockerfile.web -t infraframe/web:1.0 .

docker save infraframe/audio_agent:1.0 | gzip > audio_agent-1.0.0.tar.gz
docker save infraframe/cluster_manager:1.0 | gzip > cluster_manager-1.0.0.tar.gz
docker save infraframe/conference_agent:1.0 | gzip > conference_agent-1.0.0.tar.gz
docker save infraframe/management_api:1.0 | gzip > management_api-1.0.0.tar.gz
docker save infraframe/portal:1.0 | gzip > portal-1.0.0.tar.gz
docker save infraframe/recording_agent:1.0 | gzip > recording_agent-1.0.0.tar.gz
docker save infraframe/streaming_agent:1.0 | gzip > streaming_agent-1.0.0.tar.gz
docker save infraframe/video_agent:1.0 | gzip > video_agent-1.0.0.tar.gz
docker save infraframe/webrtc_agent:1.0 | gzip > webrtc_agent-1.0.0.tar.gz
