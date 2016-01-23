#!/bin/bash

if test $# -lt 1 ; then
  echo "usage: ./rinstall.sh IP_ADDR"
  echo "Please specify the destermination ip address."
  exit 1
fi

IP=$1



HOST=root@${IP}

echo "Installing kernel uImage into ${HOST}"
rsync -av deploy/uboot/ ${HOST}:/boot/uboot/

echo "Installing kernel modules into ${HOST}"
rsync -av --delete deploy/lib/modules/ ${HOST}:/lib/modules/
rsync -av --delete deploy/lib/firmware/ ${HOST}:/lib/firmware/

echo "Rebooting remote host ${HOST}"
ssh ${HOST} reboot

