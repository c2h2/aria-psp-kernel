#!/bin/sh
TGT=drivers/video/logo/logo_linux_clut224.ppm
if [ -z "$1" ]; then
  echo "Missing source file, usage:\n./make_logo.sh source_file.png"
  exit 1
fi

echo "Converting $1 PNG file to $TGT"

#ppmquant 224 $1 > temp_224.ppm
#pnmnoraw temp_224.ppm > logo_linux_clut224.ppm
#echo "Copying to drivers/video/logo ... "
#cp logo_linux_clut224.ppm drivers/video/logo

#require imagemagick
convert $1 -resize 800x1280\! -colors 224 -compress none $TGT
exit 0

