flash : oled.bin
	particle flash oled1 oled.bin

oled.bin : src/oled.ino \
		src/Adafruit_GFX.cpp \
		src/Adafruit_GFX.h \
		src/Adafruit_SSD1306.cpp \
		src/Adafruit_SSD1306.h \
		src/addresses.h \
		src/button.h \
		src/LedHeartbeat.h \
		src/MqttHeartbeat.h
	particle compile photon . --saveTo oled.bin

print : src/oled.ino
	vim -c 'hardcopy > output.ps'  -c quit src/oled.ino && ps2pdf output.ps >output.pdf
