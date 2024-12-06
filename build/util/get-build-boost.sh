#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
source "./.functions.sh"

BOOST_DOWNLOAD_URL="https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz"
INSTALL_PATH=${ESTATE_PLATFORM_DIR}/native/deps/boost

if [ -d "${INSTALL_PATH}" ]; then
    error "Boost already installed at ${INSTALL_PATH}"
    exit 1
fi

echo "Downloading boost"
rm -rf /tmp/boost
mkdir -p /tmp/boost
wget -q -O /tmp/boost/pkg.tar.gz ${BOOST_DOWNLOAD_URL} > /dev/null

echo "Extracting boost"
cd /tmp/boost
tar zxvf ./pkg.tar.gz > /dev/null

echo "Configuring boost"
mkdir -p ${INSTALL_PATH}

cd /tmp/boost/boost_*
./bootstrap.sh --prefix=${INSTALL_PATH} --with-toolset=gcc --with-libraries=chrono,timer,thread,system,filesystem,regex,program_options    

echo "Compiling boost"
./b2 -q install --prefix=${INSTALL_PATH}
cd
rm -rf /tmp/boost