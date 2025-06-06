#!/bin/bash -e

pause() {
  read -p "$*"
}

check_proxy() {
  if [ -z "$http_proxy" ]; then
    echo "No http proxy set, doing nothing"
  else
    echo "http proxy configured, configuring npm"
    npm config set proxy $http_proxy
  fi

  if [ -z "$https_proxy" ]; then
    echo "No https proxy set, doing nothing"
  else
    echo "https proxy configured, configuring npm"
    npm config set https-proxy $https_proxy
  fi
}

install_fdkaac() {
  local VERSION="0.1.6"
  local SRC="fdk-aac-${VERSION}.tar.gz"
  local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
  local SRC_MD5SUM="13c04c5f4f13f4c7414c95d7fcdea50f"

  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libfdk* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "fdkaac already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libfdkaac - Yes"
    else
      echo "libfdkaac - No"
    fi
    return 0
  fi

  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  [[ ! -s ${SRC} ]] && curl -L --proxy socks5h://localhost:30000 -o ${SRC} ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check); then
    rm -f ${SRC} && curl -L --proxy socks5h://localhost:30000 -o ${SRC} ${SRC_URL} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr fdk-aac-${VERSION}
  tar xf ${SRC}
  pushd fdk-aac-${VERSION}
  ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static
  make -j4 -s V=0 && make install
  popd
  popd
}

install_ffmpeg() {
  local VERSION="4.4.1"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="9c2ca54e7f353a861e57525ff6da335b"

  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libav* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "ffmpeg already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "ffmpeg - Yes"
    else
      echo "ffmpeg - No"
    fi
    return 0
  fi

  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  [[ ! -s ${SRC} ]] && curl -L --proxy socks5h://localhost:30000 -o ${SRC} ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check); then
    echo "Downloaded file ${SRC} is corrupted."
    rm -v ${SRC}
    return 1
  fi
  rm -fr ${DIR}
  tar xf ${SRC}
  pushd ${DIR}
  [[ "${DISABLE_NONFREE}" == "true" ]] &&
    PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype ||
    PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype --enable-libfdk-aac --enable-nonfree &&
    make -j4 -s V=0 && make install
  popd
  popd
}

install_zlib() {
  local VERSION="1.3.1"

  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libz* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "zlib already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "zlib - Yes"
    else
      echo "zlib - No"
    fi
    return 0
  fi

  if [ -d $LIB_DIR ]; then
    pushd $LIB_DIR >/dev/null
    rm -rf zlib-*
    rm -f ./build/lib/zlib.*
    curl -L --proxy socks5h://localhost:30000 -o zlib-${VERSION}.tar.gz https://zlib.net/zlib-${VERSION}.tar.gz
    tar -zxf zlib-${VERSION}.tar.gz
    pushd zlib-${VERSION} >/dev/null
    ./configure --prefix=$PREFIX_DIR
    make && make install
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_zlib
  fi
}

#libnice depends on zlib
install_libnice0114() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libnice* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "libnice already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libnice - Yes"
    else
      echo "libnice - No"
    fi
    return 0
  fi

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -f ./build/lib/libnice.*
    rm -rf libnice-0.1.*
    # wget -c http://nice.freedesktop.org/releases/libnice-0.1.14.tar.gz
    curl -L --proxy socks5h://localhost:30000 -o libnice-0.1.16.tar.gz https://repository.timesys.com/buildsources/l/libnice/libnice-0.1.16/libnice-0.1.16.tar.gz
    tar -zxvf libnice-0.1.16.tar.gz
    cd libnice-0.1.16
    #patch -p1 < $PATHNAME/patches/libnice-0114.patch
    #patch -p1 < $PATHNAME/patches/libnice-0001-Remove-lock.patch
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice0114
  fi
}

#libnice depends on zlib
install_libnice014() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libnice* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "libnice already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libnice - Yes"
    else
      echo "libnice - No"
    fi
    return 0
  fi

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -f ./build/lib/libnice.*
    rm -rf libnice-0.1.*
    curl -L --proxy socks5h://localhost:30000 -o libnice-0.1.4.tar.gz https://repository.timesys.com/buildsources/l/libnice/libnice-0.1.4/libnice-0.1.4.tar.gz
    tar -zxvf libnice-0.1.4.tar.gz
    cd libnice-0.1.4
    patch -p1 <$PATHNAME/patches/libnice014-agentlock.patch
    patch -p1 <$PATHNAME/patches/libnice014-agentlock-plus.patch
    patch -p1 <$PATHNAME/patches/libnice014-removecandidate.patch
    patch -p1 <$PATHNAME/patches/libnice014-keepalive.patch
    patch -p1 <$PATHNAME/patches/libnice014-startcheck.patch
    patch -p1 <$PATHNAME/patches/libnice014-closelock.patch
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice014
  fi
}

install_openssl() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libssl* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "openssl already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "openssl - Yes"
    else
      echo "openssl - No"
    fi
    return 0
  fi

  if [ -d $LIB_DIR ]; then
    local SSL_BASE_VERSION="3.2.0"
    local SSL_VERSION="3.2.0"
    cd $LIB_DIR
    rm -f ./build/lib/libssl.*
    rm -f ./build/lib/libcrypto.*
    rm -rf openssl-1*

    curl -L --proxy socks5h://localhost:30000 -o openssl-${SSL_VERSION}.tar.gz https://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
    tar xf openssl-${SSL_VERSION}.tar.gz
    cd openssl-${SSL_VERSION}
    ./config no-ssl3 --prefix=$PREFIX_DIR -fPIC --libdir=lib
    make depend
    make -s V=0
    make install

    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_openssl
  fi
}

install_openh264() {
  local LIST_LIBS=$(ls ${ROOT}/third_party/openh264/libopenh264* 2>/dev/null)
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "openh264 already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "openh264 - Yes"
    else
      echo "openh264 - No"
    fi
    return 0
  fi

  MAJOR=2
  MINOR=6
  SOVER=8

  rm $ROOT/third_party/openh264 -rf
  mkdir $ROOT/third_party/openh264

  cd $ROOT/third_party/openh264

  # license
  curl -L --proxy socks5h://localhost:30000 -o BINARY_LICENSE.txt https://www.openh264.org/BINARY_LICENSE.txt

  SOURCE=v${MAJOR}.${MINOR}.0.tar.gz

  # download source code
  curl -L --proxy socks5h://localhost:30000 -o ${SOURCE} https://github.com/cisco/openh264/archive/refs/tags/${SOURCE}

  # extract source code
  tar -zxf ${SOURCE} --strip-components=1

  # build from source
  make -j$(nproc) ENABLE_STATIC=on ENABLE_SHARED=on

  cd $CURRENT_DIR
}

install_libexpat() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libexpat* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libexpat - Yes"
    else
      echo "libexpat - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "libexpat already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local VERSION="2.4.4"
    local DURL="https://github.com/libexpat/libexpat/releases/download/R_2_4_4/expat-${VERSION}.tar.bz2"
    pushd ${LIB_DIR} >/dev/null
    rm -rf expat-*
    rm -f ./build/lib/libexpat.*
    curl -L --proxy socks5h://localhost:30000 -o expat-${VERSION}.tar.bz2 $DURL
    tar jxf expat-${VERSION}.tar.bz2
    pushd expat-${VERSION} >/dev/null
    ./configure --prefix=${PREFIX_DIR} --with-docbook --without-xmlwf
    make && make install
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_libexpat
  fi
}

install_webrtc88() {
  local OWT_WEBRTC_PATH="$ROOT/third_party/webrtc-m88"
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ -s $OWT_WEBRTC_PATH/libwebrtc.a ]]; then
      echo "webrtc88 - Yes"
    else
      echo "webrtc88 - No"
    fi
    return 0
  fi

  [ "$INCR_INSTALL" = true ] && [[ -s $OWT_WEBRTC_PATH/libwebrtc.a ]] &&
    echo "libwebrtc already installed." && return 0

  [[ ! -d $OWT_WEBRTC_PATH ]] &&
    mkdir $OWT_WEBRTC_PATH

  pushd $OWT_WEBRTC_PATH >/dev/null
  . $PATHNAME/installWebrtc.sh
  popd >/dev/null
}

install_webrtc() {
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ -s $ROOT/third_party/webrtc/libwebrtc.a ]]; then
      echo "libwebrtc - Yes"
    else
      echo "libwebrtc - No"
    fi

    if [[ -s $ROOT/third_party/webrtc-m88/libwebrtc.a ]]; then
      echo "libwebrtc m88 - Yes"
    else
      echo "libwebrtc m88 - No"
    fi
    return 0
  fi

  [ "$INCR_INSTALL" = true ] && [[ -s $ROOT/third_party/webrtc/libwebrtc.a ]] &&
    [[ -s $ROOT/third_party/webrtc-m88/libwebrtc.a ]] &&
    echo "libwebrtc already installed." && return 0

  rm -rf $ROOT/third_party/webrtc
  rm -rf $ROOT/third_party/webrtc-m88

  pushd ${ROOT}/third_party/
  ALL_PROXY=socks5h://localhost:30000 git clone git@github.com:infraframe/infraframe-deps.git

  mv infraframe-deps/webrtc ./
  mv infraframe-deps/webrtc-m88 ./
  rm -rf infraframe-deps

  ARCH=$(dpkg-architecture -q DEB_BUILD_GNU_CPU)
  cp webrtc-m88/${ARCH}/libwebrtc.a webrtc-m88/
  cp webrtc/${ARCH}/libwebrtc.a webrtc/

  popd
}

install_licode() {
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ -d ${ROOT}/third_party/licode ]]; then
      echo "licode - Yes"
    else
      echo "licode - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ -d ${ROOT}/third_party/licode ]] &&
    echo "licode already installed." && return 0

  local COMMIT="8b4692c88f1fc24dedad66b4f40b1f3d804b50ca" #v6
  local LINK_PATH="$ROOT/source/agent/webrtc/webrtcLib"
  pushd ${ROOT}/third_party >/dev/null
  rm -rf licode
  ALL_PROXY=socks5h://localhost:30000 git clone https://github.com/lynckia/licode.git
  pushd licode >/dev/null
  git reset --hard $COMMIT

  local GIT_EMAIL=$(git config --get user.email)
  local GIT_USER=$(git config --get user.name)
  [[ -z $GIT_EMAIL ]] && git config --global user.email "you@example.com"
  [[ -z $GIT_USER ]] && git config --global user.name "Your Name"

  # APPLY PATCH
  git am $PATHNAME/patches/licode/*.patch

  popd >/dev/null
  popd >/dev/null
}

install_quic() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libowt_web_transport.so 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "quic - Yes"
    else
      echo "quic - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "quic already installed." && return 0

  # QUIC IO
  rm $ROOT/third_party/quic-lib -rf
  mkdir $ROOT/third_party/quic-lib

  pushd ${ROOT}/third_party/quic-lib
  curl -L --proxy socks5h://localhost:30000 -o dist.tgz https://github.com/open-webrtc-toolkit/owt-deps-quic/releases/download/v0.1/dist.tgz
  tar xzf dist.tgz
  popd

  # WebTransport
  if [ "$ENABLE_WEBTRANSPORT" == "true" ]; then
    local QUIC_SDK_PACKAGE_NAME="quic-sdk-ubuntu-x64-ci"
    local QUIC_SDK_URL=$(cat ${ROOT}/source/agent/addons/quic/quic_sdk_url)
    local QUIC_TRANSPORT_PATH=${ROOT}/third_party/quic-transport
    local QUIC_HEADERS_DIR=${ROOT}/build/libdeps/build/include/owt
    pushd ${QUIC_TRANSPORT_PATH}
    if [ ! -d ${QUIC_HEADERS_DIR} ]; then
      mkdir -p ${QUIC_HEADERS_DIR}
    fi
    if [ ! -d ${ROOT}/build/libdeps/build/lib ]; then
      mkdir -p ${ROOT}/build/libdeps/build/lib
    fi
    if [ -d bin/release ]; then
      cp bin/release/libowt_web_transport.so ${ROOT}/build/libdeps/build/lib
      if [ -d ${QUIC_HEADERS_DIR} ]; then
        rm -r ${QUIC_HEADERS_DIR}
      fi
      cp -r include/owt ${QUIC_HEADERS_DIR}
    else
      read -p "Unable to find prebuild QUIC SDK. Please download it from ${QUIC_SDK_URL} with a GitHub personal access token or compile it from https://github.com/open-webrtc-toolkit/owt-sdk-quic."
    fi
    popd
  fi
}

install_nicer() {
  local COMMIT="24d88e95e18d7948f5892d04589acce3cc9a5880"
  pushd ${ROOT}/third_party >/dev/null
  rm -rf nICEr
  ALL_PROXY=socks5h://localhost:30000 git clone https://github.com/lynckia/nICEr.git
  pushd nICEr >/dev/null
  git reset --hard $COMMIT
  cmake -DCMAKE_C_FLAGS="-std=c99" -DCMAKE_INSTALL_PREFIX:PATH=${ROOT}/third_party/nICEr/out
  make && make install
  popd >/dev/null
  popd >/dev/null
}

install_libsrtp2() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libsrtp2* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libsrtp2 - Yes"
    else
      echo "libsrtp2 - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "libsrtp2 already installed." && return 0

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -rf libsrtp-2.1.0
    curl -o libsrtp-2.1.0.tar.gz https://codeload.github.com/cisco/libsrtp/tar.gz/v2.1.0
    tar -zxvf libsrtp-2.1.0.tar.gz
    cd libsrtp-2.1.0
    CFLAGS="-fPIC" ./configure --enable-openssl --prefix=$PREFIX_DIR --with-openssl-dir=$PREFIX_DIR
    make $FAST_MAKE -s V=0 && make uninstall && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libsrtp2
  fi
}

install_node() {
  local NODE_VERSION="v14"
  NVM_DIR="${HOME}/.nvm"

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ -s "${NVM_DIR}/nvm.sh" ]]; then
      echo "node - Yes"
    else
      echo "node - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ -s "${NVM_DIR}/nvm.sh" ]] &&
    echo "node already installed." && return 0

  echo -e "\x1b[32mInstalling nvm...\x1b[0m"
  # install nvm
  bash "${PATHNAME}/install_nvm.sh"
  #install node
  [[ -s "${NVM_DIR}/nvm.sh" ]] && . "${NVM_DIR}/nvm.sh"
  echo -e "\x1b[32mInstalling node ${NODE_VERSION}...\x1b[0m"
  nvm install ${NODE_VERSION}
  nvm use ${NODE_VERSION}
}

install_node_tools() {
  local installed=0
  if [ "$CHECK_INSTALL" = true ]; then
    npm list -g node-gyp@6.1.0 >/dev/null 2>&1 || installed=1
    if [ $installed -eq 0 ]; then
      echo "node_tools - Yes"
    else
      echo "node_tools - No"
    fi
    return 0
  fi
  if [ "${INCR_INSTALL}" == "true" ]; then
    npm list -g node-gyp@6.1.0 >/dev/null 2>&1 || installed=1
    [ $installed -eq 0 ] && echo "node tools already installed." && return 0
  fi

  check_proxy
  npm install -g --loglevel error node-gyp@6.1.0 grunt-cli underscore jsdoc
  pushd ${ROOT} >/dev/null
  npm install nan@2.15.0
  pushd ${ROOT}/node_modules/nan >/dev/null
  patch -p1 <$PATHNAME/patches/nan.patch
  popd >/dev/null
  popd >/dev/null
}

# libre depends on openssl
install_libre() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libre* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "libre - Yes"
    else
      echo "libre - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "libre already installed." && return 0

  if [ -d $LIB_DIR ]; then
    pushd ${LIB_DIR} >/dev/null
    rm -rf re
    ALL_PROXY=socks5h://localhost:30000 git clone https://github.com/creytiv/re.git
    pushd re >/dev/null
    git checkout v0.5.0
    make SYSROOT_ALT=${PREFIX_DIR} RELEASE=1
    make install SYSROOT_ALT=${PREFIX_DIR} RELEASE=1 PREFIX=${PREFIX_DIR}
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_libre
  fi
}

install_usrsctp() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libusrsctp* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "usrsctp - Yes"
    else
      echo "usrsctp - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "usrsctp already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local USRSCTP_VERSION="0.9.5.0"
    local USRSCTP_FILE="${USRSCTP_VERSION}.tar.gz"
    local USRSCTP_EXTRACT="usrsctp-${USRSCTP_VERSION}"
    local USRSCTP_URL="https://github.com/sctplab/usrsctp/archive/${USRSCTP_FILE}"

    cd $LIB_DIR
    rm -rf usrsctp
    curl -L --proxy socks5h://localhost:30000 -o ${USRSCTP_FILE} ${USRSCTP_URL}
    tar -zxvf ${USRSCTP_FILE}
    mv ${USRSCTP_EXTRACT} usrsctp
    rm ${USRSCTP_FILE}

    cd usrsctp
    ./bootstrap
    ./configure --prefix=$PREFIX_DIR
    # CFLAGS="-Wno-address-of-packed-member"
    make && make install
  else
    mkdir -p $LIB_DIR
    install_usrsctp
  fi
}

install_glib() {
  if [ -d $LIB_DIR ]; then
    local VERSION="2.54.1"
    cd $LIB_DIR
    curl -L --proxy socks5h://localhost:30000 -o glib-${VERSION}.tar.gz https://github.com/GNOME/glib/archive/${VERSION}.tar.gz

    tar -xvzf glib-${VERSION}.tar.gz
    cd glib-${VERSION}
    ./autogen.sh --enable-libmount=no --prefix=${PREFIX_DIR}
    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:$LD_LIBRARY_PATH make && make install
  else
    mkdir -p $LIB_DIR
    install_glib
  fi
}

install_gcc() {
  if [ -d $LIB_DIR ]; then
    local VERSION="4.8.4"
    cd $LIB_DIR
    curl -L --proxy socks5h://localhost:30000 -o gcc-${VERSION}.tar.bz2 http://ftp.gnu.org/gnu/gcc/gcc-${VERSION}/gcc-${VERSION}.tar.bz2

    tar jxf gcc-${VERSION}.tar.bz2
    cd gcc-${VERSION}
    ./contrib/download_prerequisites
    make distclean
    ./configure --prefix=${PREFIX_DIR} -enable-threads=posix -disable-checking -disable-multilib -enable-languages=c,c++ --disable-bootstrap

    if
      [ $? -eq 0 ]
    then
      echo "this gcc configure is success"
    else
      echo "this gcc configure is failed"
    fi

    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:$LD_LIBRARY_PATH make -s && make install

    [ $? -eq 0 ] && echo install success && export CC=${PREFIX_DIR}/bin/gcc && export CXX=${PREFIX_DIR}/bin/g++
  else
    mkdir -p $LIB_DIR
    install_gcc
  fi
}

install_json_hpp() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/include/json.hpp 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "json_hpp - Yes"
    else
      echo "json_hpp - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "json_hpp already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local DOWNLOAD_JSON_LINK="https://github.com/nlohmann/json/releases/download/v3.6.1/json.hpp"
    pushd $LIB_DIR >/dev/null
    curl -L --proxy socks5h://localhost:30000 -o json.hpp ${DOWNLOAD_JSON_LINK}
    mkdir -p ${PREFIX_DIR}/include
    mv json.hpp ${PREFIX_DIR}/include/
    popd
  else
    mkdir -p $LIB_DIR
    install_json_hpp
  fi
}

install_svt_hevc() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libSvtHevcEnc* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "svt_hevc - Yes"
    else
      echo "svt_hevc - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "svt_hevc already installed." && return 0

  pushd $ROOT/third_party >/dev/null
  rm -rf SVT-HEVC*
  # ALL_PROXY=socks5h://localhost:30000 git clone https://github.com/intel/SVT-HEVC.git

  # pushd SVT-HEVC >/dev/null
  # git checkout v1.5.1

  # if [[ "$OS" =~ .*centos.* ]]; then
  #   source /opt/rh/devtoolset-7/enable
  # fi

  curl -L --proxy socks5h://localhost:30000 -o SVT-HEVC.tar.gz https://github.com/OpenVisualCloud/SVT-HEVC/archive/refs/tags/v1.5.1.tar.gz
  tar -xzf SVT-HEVC.tar.gz
  mv SVT-HEVC-1.5.1 SVT-HEVC
  pushd SVT-HEVC >/dev/null

  mkdir -p build
  pushd build >/dev/null
  cmake -DCMAKE_C_FLAGS="-std=gnu99" -DCMAKE_INSTALL_PREFIX=${PREFIX_DIR} ..
  make && make install
  popd >/dev/null

  # pseudo lib
  echo \
    'const char* stub() {return "this is a stub lib";}' \
    >pseudo-svtHevcEnc.cpp
  gcc pseudo-svtHevcEnc.cpp -fPIC -shared -o pseudo-svtHevcEnc.so

  popd >/dev/null
  popd >/dev/null
}

cleanup_common() {
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -r openssl*
    rm -r libnice*
    rm -r libav*
    rm -r libvpx*
    rm -f gcc*
    rm -f libva-utils*
    cd $CURRENT_DIR
  fi
}

install_boost() {
  local LIST_LIBS=$(ls ${PREFIX_DIR}/lib/libboost* 2>/dev/null)
  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "boost - Yes"
    else
      echo "boost - No"
    fi
    return 0
  fi
  [ "$INCR_INSTALL" = true ] && [[ ! -z $LIST_LIBS ]] &&
    echo "boost already installed." && return 0

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    # curl -L --proxy socks5h://localhost:30000 -o boost-1.84.0.tar.xz https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
    # tar xvf boost-1.84.0.tar.xz
    cd boost-1.84.0
    chmod +x bootstrap.sh
    ./bootstrap.sh
    ./b2 && ./b2 install --prefix=$PREFIX_DIR
  else
    mkdir -p $LIB_DIR
    install_boost
  fi
}
