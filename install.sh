#!/bin/bash

# global DEV_SEL will disable menu selection
DEV="${DEV_SEL:-}"

source helper.sh

select_disk()
{
	source tui.sh
	
	TOP_LANE=2
	TITLE="Select disk to install kernel:"
	
	disks=`lsblk  | grep ^sd | grep -v a | awk -F ' ' '{print $1}'`
	
	MENU_OPTIONS=( ${disks} )
	TITLE="multiple selection"
	show_menu_quit
	DEV="/dev/${MENU_OPTIONS[$?]}"
	printf "select " 
	tput setaf 2
	printf "   %s  " "${DEV}"
	tput sgr0
	printf " to  install\n"
	read -p "Are you sure to continue?(y/n) default y:  " yn
	
	if test "$yn" = "n" ; then
	    echo "user cancelled"
        exit 1
	fi
	
}

if test "$DEV" = "" ; then
    select_disk
fi


DEV_BOOT=${DEV}1
DEV_ROOT=${DEV}2
HASH=md5sum
MNT=`mktemp -d`
source "./build.sh" #to get other system variables

#set -e

######### boot fs ##########
install_kernel(){
  echo_stage "#####Begin installing kernel: $DATE #########"
  mount $DEV_BOOT $MNT

  HASH_ORGU=`$HASH $MNT/uImage`
  #HASH_ORGZ=`$HASH $MNT/zImage`
  echo "Original $HASH of $MNT/uImage = $HASH_ORGU"
  #echo "Original $HASH of $MNT/zImage = $HASH_ORGZ"

  cp $DEPLOY/uboot/uImage $MNT
  #cp $DEPLOY/uboot/zImage $MNT 

  echo_stage "copied, syncing to disk."
  sync 
  check_warning_and_success $? "" "warning sync failed"

  #double check if installed correctly.
  HASH_NEWU=`$HASH $MNT/uImage`
  #HASH_NEWZ=`$HASH $MNT/zImage`

  echo "Newly Deployed $HASH of $MNT/uImage = $HASH_NEWU"
  #echo "Newly Deployed $HASH of $MNT/zImage = $HASH_NEWZ"
  
  if [ "${HASH_ORGU}" = "${HASH_NEWU}" ] ; 
  then
      echo_warning "===== WARNNING: same image copied. ======"
  else
      echo_success "===== OKAY: Kernel updated and completed successfully. ======";
  fi

  umount $MNT
  check_warning_and_success $? "" "warning umount $MNT failed"
}


######### root fs ##########
install_kernel_modules(){
  echo_stage "#### Begin installing kernel modules"

  mount $DEV_ROOT $MNT
  rm -rf $MNT/lib/modules/* #clean old kernel modules.
  cp -rv $DEPLOY/lib $MNT
  sync
  check_warning_and_success $? "" "warning sync failed"
  umount $MNT
  check_warning_and_success $? "" "warning umount $MNT failed"

  echo_success "===== OKAY: Kernel modules copied successfully. ====="
}

start_time=$(date +%s)

install_kernel
install_kernel_modules

  finish_time=$(date +%s)
echo_stage "Total Time Cost for installation: $((finish_time - start_time)) seconds."

