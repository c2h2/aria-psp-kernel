Ariaboard Kernel
================

Init from ti ezsdk v0.6.0, linux-3.2.0-psp04.06.00.11 sdk

This git is Linux 3.2 kernel source files Aria Board, Starts from $39 Texas Instruments Coretex-A8 Stamp Board.

For more info, plz visit: [Aria Board Homepage](http://ariaboard.com/) 


To Build
========
Install cross compiling for AM335x: http://ariaboard.com/wiki/Easy_Install_gcc_Cross_Compiler

Clone the source and 

    git clone https://github.com/c2h2/aria-psp-kernel.git
    cd aria-psp-kernel  
    ./mproperbuild.sh

To Install to a SD Card Locally
===============================
    ./install.sh


To Install to a SD Card Remotely
================================
    ./rinstall.sh remoteIP
