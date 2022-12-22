#!/usr/bin/python3
from subprocess import run
import shlex
import sys
from glob import glob
import os
from pprint import pprint
import struct
import json

width = 100
height = 75

wckeys = [ 'qa', 'ec', 'sn', 'nl',
           'gb-eng', 'ir', 'us', 'gb-wls',
           'ar', 'sa', 'mx', 'pl',
           'fr', 'au', 'dk', 'tn',
           'es', 'cr', 'de', 'jp',
           'be', 'ca', 'ma', 'hr',
           'br', 'rs', 'ch', 'cm',
           'pt', 'gh', 'uy', 'kr'
          ]

cmd = "unzip flagma-main.zip"
cmdarr = shlex.split(cmd)
run(cmdarr)

flagdir = "flagma-main/flags"
ddir    = "rgb"
os.makedirs(ddir, exist_ok=True)

# calculate the offsets
fw = open( os.path.join("tftflags.bin"), 'bw+') 
offacc = 0
offarr = []
fdict = {}
os.makedirs( "rgb", exist_ok=True )
csiz = width * height * 2

for key in wckeys:

    cfile = os.path.join(flagdir, key + ".svg")
    tfile = os.path.join("rgb", key + ".rgb")
    cmd = "convert " + cfile + " -resize " + str(width) + "x" + str(height) + " " + tfile
    cmdarr = shlex.split( cmd )
    run( cmdarr )
    #c = resdict[key]
    fdict[key] = {}
    fdict[key]['offset'] = offacc
    offarr.append( offacc )
    fdict[key]['size']   = csiz
    offacc += csiz
    npix = width * height
    flag = "rgb/" + key + ".rgb"
    fr = open( flag, "rb" )
    for pix in range( 0,npix ):
        ro = int.from_bytes(fr.read(1),"big")
        go = int.from_bytes(fr.read(1),"big")
        bo = int.from_bytes(fr.read(1),"big")
        r = int(ro / 256.0 * 32)
        g = int(go / 256.0 * 64)
        b = int(bo / 256.0 * 32)        
        rgb = (r << 11) + (g << 5) + b
        fw.write( struct.pack("<H",rgb))
    fr.close()
fw.close()

pprint(fdict);

df = open(  os.path.join(ddir, "flags.json"), 'w+') 
json.dump( fdict,df)
df.close()
print()
print("Line to copy in the program for the flag offsets:\n")
print( "uint32_t offsets[] = {", end="" )
for off in offarr:
    print( "%d " % off, end="")
print(" }")
print()


print("Command to load the flags into the RP2040")
print("sudo picotool load tftflags.bin -o 0x%x -f" % 0x10100000)
print()
