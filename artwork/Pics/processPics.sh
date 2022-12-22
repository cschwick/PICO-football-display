#!/usr/bin/bash

mkdir -p tmp

convert png/ball_02.png -resize 320x240 -background "#eeeeee" -gravity center -extent 320x240 tmp/ball.rgb

convert png/coup_01_320.png -resize 320x240 -background "#eeeeee" -gravity center -extent 320x240 tmp/coup.rgb

convert png/stadium_01_320.png tmp/stadium.rgb

if [ -f pictures.bin ]; then
    rm pictures.bin
fi
#offset=0x10175300
offset=269964032
for pic in tmp/stadium.rgb tmp/ball.rgb tmp/coup.rgb
do
    rgb24_16.py $pic
    fn=${pic::-4}_16bit.rgb
    fs=`stat --printf="%s" $fn`
    printf "%25s %6d offset 0x%08x\n" $fn $fs $offset
    offset=$(( $offset + $fs ))
    cat $fn >> pictures.bin
done

printf "first free byte 0x%08x\n" $offset

echo "To load the binary picture data in the RP2040 use:"
echo "sudo picotool load pictures.bin -o 0x10175300 -f "
