#!/usr/bin/python3

import serial
import sys
import stat
import os
from time import sleep
from pprint import pprint

# open the serial port

SECTORFILE = "sector.bin"

sername = "/dev/ttyACM0"
ser = serial.Serial(
    port = sername,
#    baudrate = 57600
)

def writeSector( sector ):
    fd = open(SECTORFILE, "wb")
    for i in range(0,4096):
        fd.write( bytes([ sector[i]] ))
    fd.close()

if not os.path.isfile( SECTORFILE ):
    fd = open(SECTORFILE, "wb" )
    for i in range(0,4096):
        fd.write( bytes([0xff]) )
    fd.close()


sector = bytearray()

fd = open(SECTORFILE, "rb")
for i in range(0,4096):
    sector += fd.read(1)
fd.close()

#while True:
#    inb = ser.readline()
#    print( inb )

while True:
    inb = ser.read(size=1)
    print( repr(inb) )
    if inb[0] == 114:
        print("sending sector")
        ser.write(sector);
        print("done")
        while True:
            inb = ser.readline()
            print(inb)
        
    elif inb[0] == 101: # e
        print( "erasing sector")
        for i in range(0,4096):
            sector[i] = 0xff
            writeSector( sector )
    elif inb[0] == 119: # w
        inb = ser.read(size=1)
        off = inb[0]*256;
        print( "receiving page %d 0ffset 0x%04x" %(inb[0], off))
        for i in range(off,off + 256):
            inb = ser.read(size=1)
            #print(inb)
            sector[i] = inb[0]
        writeSector(sector)
    else:
        print("Did not get valid command: %d  0x%02x" % (inb[0], inb[0]))
    
