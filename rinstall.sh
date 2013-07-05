#!/bin/bash

if test $# -lt 1 ; then
  echo "usage: ./rinstall.sh IP_ADDR"
  echo "Please specify the destermination ip address."
  exit 1
fi

IP=$1



HOST=root@${IP}

echo "Installing kernel uImage into ${HOST}"
rsync -avz deploy/uboot/ ${HOST}:/boot/uboot/

echo "Installing kernel modules into ${HOST}"
rsync -avz --delete deploy/lib/modules/ ${HOST}:/lib/modules/

echo "Rebooting remote host ${HOST}"
ssh ${HOST} reboot

