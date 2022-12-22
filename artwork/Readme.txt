==============================================================================
Pictures:
==============================================================================

processPics.sh: takes the png files and resizes them to 320x240 rbg565 files.
They are then concatenated and to a file pictures.bin and the offsets are
printed out. 

rgb24_16.py {filename} : converts a 24 bit rgb-file to a 16-bit rgb file.  


usage:
cd Pics
./processPics.sh
sudo picotool load pictures.bin -o 0x10175300 -f








==============================================================================
Flags
==============================================================================

cd flags
python3 makePicodata.py
