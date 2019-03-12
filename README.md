# oled4

## Welcome to the OLED display controller
## -- for the MQTT/Node RED demonstration project


###Objectives of this project:

- build an independent network of IoT devices
that can communicate with each other via a central node.
- demonstrate a local IoT network not connected to the Internet.
- demonstrate how Node-RED can be used to control aspects of the network
- (security is not currently part of this demonstration network)

###Requirements for each node:

The basic requirements of each node are:

- devices have basic input and/or output capabilities, but otherwise are dumb
devices.
- communication with particle.io, is NOT done unless a switch is closed. The
switch allows device development with the particle processes.
- communication is normally with publish/subscribe messages to and from an MQTT
broker hardwired at 192.168.4.1.

###Future dream requirements for each node:

The requirements of each node in a future phase are:

- all devices have a discovery command to learn their capabilities.
A json file is returned with a list of configurable attributes.
- all (or most) devices can be configured as necessary to support their internal
functions.
- all devices have a heartbeat so that their presence or failure can be passively
monitored.
- all devices have a ping and ping-response so that their presence or failure
can be actively monitored.
- devices connecting via WiFi discover the MQTT broker with a url like:
http://mqttbroker.local:1882.


### Current Status of the OLED Controller

 1. [/] experiment with turning off Particle.io stuff

- [/] add a button for semi automatic or manual operation
- [/] objectify the slots
- [/] objectify the buttons
- [ ] try to use the u8g2 driver instead of Adafruit
- [ ] add in code for status

    * booting
    * no serial
    * attempting wifi connection
    * WiFi connected and local IP address
    * WiFi connected and Internet accessed
    * attempting MQTT connection
    * MQTT connected
    * other fault

-  [ ] want to be able to assign slot to a particular functions
    * time
    * IP address
    * connection status
    * MQTT bus

- [/] serial logging needs to be a bit more consistent
    * [/] always include time of event to seconds
    * [/] always start with Capital letter
    * [/] expiring time should include seconds
    * [/] strings should be enclosed in quotes

- [ ] fix local time


## Requirements for the OLED display

The OLED display is an array of 128 x 32 pixels (available from AdaFruit.com).
In general it is thought to be a textual display rather than a
graphical display. The current implementation
has a set of text slots, onto which can be displayed a text message.
Slots 1-4 are very small fonts that can be displayed at the same time.
Slot 5-7 are bigger fonts and overlap the smaller fonts and therefore
take priority over the smaller fonts.

- Slot 5 overlaps Slots 1 and 2
- Slot 6 overlaps Slots 2 and 3
- Slot 7 overlaps Slots 3 and 5

A slot is erased by updating it with a null string, "".

Built in information can be configured to be displayed in any slot.
By default connection status is displayed in slot 5,
the node IP address is displayed in slot 3.
time is displayed in slot 4.


###Phase 1:
- [ ] print subscribed messages
    - [ ] need to assume preformated strings
    - [ ] need a caption and a value
(e.g.: "Out T: 98F")
    - [ ] position? x,y
    - [ ] size?
    - [ ] inverted?
-  [ ] how complicated of a message can we handle?
-  [ ] publish button messages
-  [ ] publish change of states
    - [ ] a heartbeat would be sufficient
-  [ ] publish a heartbeat
    - every (10?) minutes
-  [ ] timestamp published messages

-  [ ] config to invert individual slots
-  [ ] config to set time to live on a particular slot

###Phase 2:
- [ ] add some user controls
    - [ ] review subscriptions
    - [ ] select subscriptions or autoscroll
- [] mechanism to handle fonts
    - [ ] don't really want to pass font name
    - [ ] limited by memory
    - [ ] have extra flexibility possibility: normal, bold, italics, bold italics
    - [ ] different famililies: monospaced, sans serif, serif

- could just use a simple index, but for a family of devices, this matrix
maybe sparse as some fonts are included and some are not.

- The default font can be used with a magnification 1, 2, 4...
    - display.setFont(&FreeSans9pt7b);
    - display.setFont(&FreeSans12pt7b);
    - display.setFont(&FreeSans18pt7b);
    - display.setFont(&FreeSans24pt7b);

- for 32x128 display:

    - basic 9pt baselines would be at 8, 18, and 28.
    - basic 12pt baselines could be 24 + 9 = 33... 12, 24, 31
        - 18 + 12 = 30
        - 18 + 9 = 27
    - in other words there is too much flexibility to make the positioning generic


- need a way to send:
    - font family
    - font style
    - font size (font height, a bit less than field height)
    - font x (x coordinate of field baseline)
    - font y (y coordinate of field baseline)
    - width of field (pixels)
    - color (black on white or white on black)
    - if family, style or size not available, use default

#### Files

The **src/oled.ino** is the main source file for this project.

The **src/address.ino** contains the node name and MQTT broker address as
these will change for different networks.

The **lib** directory includes the header files for various libraries. The
header file for Adafruit_SSD1306 had to be modified to accomodate the
128x32 display. The header files for the fonts are there as well.

The **project.properties** file that specifies the name and version number of the libraries that this project depends on. 