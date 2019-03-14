oled.bin : src/oled.ino
	particle compile photon . --saveTo oled.bin

flash : oled.bin
	particle flash oled1 oled.bin

