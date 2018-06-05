#!/bin/sh
# cross compile arm kernel for am335x itc coreboard.

export PATH=/opt/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux/bin:$PATH

export CROSS_COMPILE=arm-linux-gnueabihf-

DATE=`date +%Y%m%d-%H%M`
LOG_FILE=kernel_build_log_${DATE}.log
CORES=`getconf _NPROCESSORS_ONLN`
DEPLOY="deploy"
PWD=`pwd`


clean_tgt(){
  cd $PWD
  rm -rf $DEPLOY
 
  cp .config config.old
  make clean ARCH=arm
  cp config.old .config
}

build_tgt(){
  start_time=$(date +%s)
  cd $PWD
  
  mkdir -p $DEPLOY
  mkdir -p $DEPLOY/uboot


  make -j$CORES uImage modules firmware ARCH=arm | tee -a $LOG_FILE
  make ARCH=arm modules_install INSTALL_MOD_PATH=$DEPLOY/
  make ARCH=arm headers_install INSTALL_MOD_PATH=$DEPLOY/
  make ARCH=arm firmware_install INSTALL_MOD_PATH=$DEPLOY/

  #add kernel headers

 # make oldconfig ARCH=arm && make prepare ARCH=arm
 # make scripts ARCH=arm


  rm -f $DEPLOY/uboot/uImage
  rm -f $DEPLOY/uboot/zImage
  
  cp arch/arm/boot/uImage $DEPLOY/uboot/
  cp arch/arm/boot/zImage $DEPLOY/uboot/

  finish_time=$(date +%s)
}

welcome_start(){
  echo "########### Cross compiling uImage $DATE #############"
}

welcome_end(){
  echo "Finished compiliation.. Time duration: $((finish_time - start_time)) secs. (with $CORES threads.)"
}

