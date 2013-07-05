#!/bin/sh

#install pre shit packages.
sudo apt-get -y install device-tree-compiler lzop u-boot-tools ia32-libs 


sudo mkdir /opt
cd /opt

sudo wget -O gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux.tar.bz2 https://launchpad.net/linaro-toolchain-binaries/trunk/2013.03/+download/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux.tar.bz2
sudo tar xvfj gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux.tar.bz2

echo 'PATH=/opt/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux/bin:${PATH}' > ~/.bashrc
