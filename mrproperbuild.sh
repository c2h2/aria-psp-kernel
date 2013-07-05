#!/bin/bash
# cross compile arm kernel for am335x itc coreboard.

source ./build.sh

welcome_start

clean_tgt
build_tgt

welcome_end
