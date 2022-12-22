#!/usr/bin/python3

import sys
import os
import struct

infile = sys.argv[1]

fr = open( infile, 'rb')
outfile = os.path.splitext(infile)[0] + "_16bit.rgb"
#print(outfile)
fw = open( outfile, 'bw+')

while 1:
    red = fr.read(1)
    #print ( type(red), repr(red)) 
    if red == b"":
        break
    red = red[0] >> 3
    green = int(fr.read(1)[0]) >> 2
    blue = int(fr.read(1)[0]) >> 3
    rgb16 = (red<<11) + (green << 5) + blue
    #print(type(rgb16), "%x"%rgb16)
    fw.write( struct.pack( "H", rgb16 ) )
