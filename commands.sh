sudo picotool load football.uf2 -f 
# backup results page (4k) (from Pico to computer)
sudo picotool save -r 0x101FF000 0x10200000 results.bin -f
#load back the results page
sudo picotool load results.bin -o 0x101FF000 -f

# back up pictures and flags onto the PC
sudo picotool save -r 0x10100000 0x101E5AFF artwork.bin -f
# load back the "artwork"
sudo picotool load artwork.bin -o 0x10100000 -f

# commands to update firmware
sudo picotool load build/football.uf2 -f

# command to load the binary picture data into the RP2040
sudo picotool load pictures.bin -o 0x10175300 -f 


# command to load the flags into the RP2040
sudo picotool load tftflags.bin -o 0x10100000 -f
