flash : oled.bin
	particle flash oled1 oled.bin

oled.bin : src/oled.ino
	particle compile photon . --saveTo oled.bin

print : src/oled.ino
	vim -c 'hardcopy > output.ps'  -c quit src/oled.ino && ps2pdf output.ps >output.pdf
