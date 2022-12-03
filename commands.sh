sudo picotool load football.uf2 -f 
# save results page (4k) (from Pico to computer)
sudo picotool save -r 0x101FF000 0x10200000 results.bin -f
#load back the results page
sudo picotool load results.bin -o 0x101FF000 -f

# save pictures and flags
sudo picotool save -r 0x10100000 0x101EF0FF artwork.bin -f
# load back the "artwork"
sudo picotool load artwork.bin -o 0x10100000 -f


# commands to update firmware and results
sudo picotool  load build/football.uf2 -f
sudo picotool load results.bin -o 0x101FF000 -f

