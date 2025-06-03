#!/bin/bash -e

install_apt_deps() {
  ${SUDO} apt update -y
  ${SUDO} apt install git make gcc g++ libglib2.0-dev docbook2x pkg-config -y
  ${SUDO} apt install openjdk-8-jre liblog4cxx-dev curl nasm yasm gyp libx11-dev libkrb5-dev -y
  ${SUDO} apt install m4 autoconf libtool automake cmake libfreetype6-dev -y
  ${SUDO} apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev -y
  if [ "$GITHUB_ACTIONS" != "true" ]; then
    ${SUDO} apt-get install rabbitmq-server mongodb -y
  else
    if [ -d $LIB_DIR ]; then
      echo "Installing mongodb-org from tar"
      ${SUDO} apt install -y libcurl4 openssl liblzma5
      wget -q -P $LIB_DIR https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1804-4.4.6.tgz
      tar -zxvf $LIB_DIR/mongodb-linux-x86_64-ubuntu1804-4.4.6.tgz -C $LIB_DIR
      ${SUDO} ln -s $LIB_DIR/mongodb-linux-x86_64-ubuntu1804-4.4.6/bin/* /usr/local/bin/
    else
      mkdir -p $LIB_DIR
      install_mongodb
    fi
  fi
}

install_gcc_7() {
  ${SUDO} apt-get install gcc-7 g++-7
  ${SUDO} update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 100
  ${SUDO} update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100
  ${SUDO} update-alternatives --set g++ /usr/bin/g++-7
  ${SUDO} update-alternatives --set gcc /usr/bin/gcc-7
}

install_mediadeps_nonfree() {
  install_fdkaac
  install_ffmpeg
}

install_mediadeps() {
  install_ffmpeg
}

cleanup() {
  cleanup_common
}
