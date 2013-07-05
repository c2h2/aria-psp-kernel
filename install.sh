#!/bin/bash

DEV=/dev/sdf

DEV_BOOT=${DEV}1
DEV_ROOT=${DEV}2
HASH=md5sum
MNT=`mktemp -d`
source "./build.sh" #to get other system variables

set -e

######### boot fs ##########
install_kernel(){
  echo "#####Begin installing kernel: $DATE #########"
  mount $DEV_BOOT $MNT

  HASH_ORGU=`$HASH $MNT/uImage`
  HASH_ORGZ=`$HASH $MNT/zImage`
  echo "Original $HASH of $MNT/uImage = $HASH_ORGU"
  echo "Original $HASH of $MNT/zImage = $HASH_ORGZ"

  cp $DEPLOY/uboot/uImage $MNT
  cp $DEPLOY/uboot/zImage $MNT 

  echo "copied, syncing to disk."
  sync 

  #double check if installed correctly.
  HASH_NEWU=`$HASH $MNT/uImage`
  HASH_NEWZ=`$HASH $MNT/zImage`

  echo "Newly Deployed $HASH of $MNT/uImage = $HASH_NEWU"
  echo "Newly Deployed $HASH of $MNT/zImage = $HASH_NEWZ"
  
  if [ "${HASH_ORGU}" = "${HASH_NEWU}" ] ; 
  then
      echo "===== WARNNING: same image copied. ======"
  else
      echo "===== OKAY: Kernel updated and completed successfully. ======";
  fi

  umount $MNT
}


######### root fs ##########
install_kernel_modules(){
  echo "#### Begin installing kernel modules"

  mount $DEV_ROOT $MNT
  rm -rf $MNT/lib/modules/* #clean old kernel modules.
  cp -rv $DEPLOY/lib $MNT
  sync
  umount $MNT

  echo "===== OKAY: Kernel modules copied successfully. ====="
}

start_time=$(date +%s)

install_kernel
install_kernel_modules

  finish_time=$(date +%s)
echo "Total Time Cost for installation: $((finish_time - start_time)) seconds."

