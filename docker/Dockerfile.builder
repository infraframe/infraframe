# 使用合适的基础镜像
FROM ubuntu:20.04

ARG TARGETARCH

# 安装常用工具和依赖
ENV TZ='Asia/Shanghai'
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
sed -i "s@http://.*.ubuntu.com@http://mirrors.tuna.tsinghua.edu.cn@g" /etc/apt/sources.list && \
apt-get update && \
apt-get install -y \
    sudo \
    wget \
    curl \
    cmake \
    build-essential \
    libssl-dev \
    zlib1g-dev \
    libexpat1-dev \
    libusrsctp-dev \
    libnice-dev \
    libglib2.0-dev \
    libjsoncpp-dev \
    libboost-all-dev \
    python3 \
    python3-pip \
    gcc-9 g++-9 \
    net-tools \
    git \
    yasm \
    unzip \
    docbook2x build-essential \
    autoconf automake libtool pkg-config curl \
    libfreetype6-dev \
    pkg-config \
    libglib2.0-dev \
    liblog4cxx-dev \
    libgstreamer-plugins-base1.0-dev \
    qt5-default qtbase5-dev \
    libcairo2-dev librsvg2-dev libglib2.0-dev libfmt-dev

# 设置 GCC 和 G++ 的默认版本
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 && \
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90 && \
update-alternatives --set g++ /usr/bin/g++-9 && \
update-alternatives --set gcc /usr/bin/gcc-9
