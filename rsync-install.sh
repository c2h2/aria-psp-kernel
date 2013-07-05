#!/usr/bin/expect

set timeout 30


#HOST=root@192.168.11.236
#DEV_PATH=$HOST:/home/ubuntu/update
#source "./build.sh"

#set -e

spawn rsync -avz deploy/uboot/ root@192.168.11.236:/boot/uboot/
expect "password:"
send "123456\r"

spawn rsync -avz deploy/lib/modules/ root@192.168.11.236:/lib/modules/
expect "password:"
send "123456\r"

spawn ssh root@192.168.11.236 reboot
expect "password:"
send "123456\r"
expect eof
exit

#install_kernel(){
#    echo "#####Begin installing kernel: $DATE #########"
#    scp -vf $DEPLOY/uImage $DEPLOY/zImage $DEV_BOOT
#    echo "copied."
#}

#install_kernel_modules(){

 #   echo "#### Begin installing kernel modules"

#    scp -rf $DEPLOY/lib/modules/* $DEV_ROOT
#    echo "===== OKAY: Kernel modules copied successfully. ====="
#}

#start_time=$(date +%s)

#install_kernel
#install_kernel_modules

#finish_time=$(date +%s)
