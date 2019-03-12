/*
 * Project oled
 * Description:  control an OLED display for generalized MQTT data
 * Author: Kirk Carlson
 * Date: 8 Aug 2018
 */

/*
Oled 2...
  want to phase in a few changes:
    1/ experiment with turning off Particle.io stuff
     / -- add a button for semi automatic or manual operation
    2/ objectify the slots
    3/ objectify the buttons
    4 try to use the u8g2 driver instead of Adafruit
    5 add in code for communication status
      - no serial
      - no wifi
      - wifi connected and local IP address
      - wifi broken and attempting reconnect
      - no MQTT broker
      - MQTT broker connect broken, attempting reconnect
      - no external internet
    6 want to be able to assign slot to local functions
      connection status
      time
      ip address

      either:
        assign slot to function (including MQTT)
        **this is easier because it fits into the rest of it
      or:
        assign function to slot

    7/ serial logging needs to be a bit more consistant
     / - always include time of event to seconds
     / - always start with Capital letter
     / - expiring time should include seconds
     / - strings should be enclosed in quotes

    8  local time is wrong




Basically want the following displayed:

status
  Booting
  Connected
  Disconnected
  Fault

Phase 1:
  print subscribed messages
    need to assume preformated strings
    need a caption and a value
      e.g.: Cola T: 98F
    position? x,y
    size?
    inverted?
  how complicated of a message can we handle?
  publish button messages /
  publish change of states
    a heartbeat would be sufficient
  publish a heartbeat
    every (10?) minutes
  timestamp published messages

  divide display into slots
     slot1 - slot4 small text slots
     slot5 - slot7 larger text slots
     slot5 overlaps slot1 and slot2
     slot6 overlaps slot2 and slot3
     slot7 overlaps slot3 and slot4
     config to invert individual slots
     config to set time to live on a particular slot

Phase 2:
  add some user controls
    review subscriptions
    select subscriptions or autoscroll




/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************

void callback(char* topic, byte* payload, unsigned int length);
*/


// *** INCLUDES ***

#include <MQTT.h>
#include <NtpTime.h>
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "FreeSans9pt7b.h"
#include "FreeSans12pt7b.h"
#include "FreeSans18pt7b.h"
#include "FreeSans24pt7b.h"

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


// *** HARDWARE DEFINES ***
// D0 -- SDA
// D1 --SCL
#define OLED_RESET D5
#define BUTTON_MANUAL D6


// *** CONSTANTS ***
//HOSTNAME is the name of the current microcontroller on the WiFi network
#define HOSTNAME "oled1"

#define ON 1
#define OFF 0
#define MAX_PAYLOAD_SIZE 100
// SLOT_EXPIRY is in seconds
#define SLOT_EXPIRY 5*60
// HEARTBEAT_PERIOD is in milliseconds
#define HEARTBEAT_PERIOD 10*60*1000
#define NO_EXPIRATION 0
#define KEEP_ALIVE 60
#define MAX_TOPIC_LENGTH 30
#define MAX_STRING_SIZE 32


// *** GLOBALS ***
// yes this is bad, three globals. Just to save memory
// there are used only as temporary variables within functions
// but they are common
String topic;
char loopTopic[ MAX_TOPIC_LENGTH + 1];
char stringBuffer[ MAX_STRING_SIZE + 1];
unsigned long  currentTime = 0;
unsigned long  heartBeatDue = 0;
bool payloadChanged; // new payload


// *** INTERFACE OBJECTS ***

NtpTime ntptime;

/**
 * if want to use IP address:
 *   byte server[] = { XXX,XXX,XXX,XXX };
 *   MQTT client(server, 1882, callback);
 * want to use domain name:
 *   MQTT client("iot.eclipse.org", 1882, callback);
 **/

byte server[] = { 192,168,4,1 };
//MQTT client( server, 1883, callback);
//need initialization verions that include keep alive timer values
MQTT client( server, 1883, KEEP_ALIVE, callback);

Adafruit_SSD1306 display(OLED_RESET);


//  *** CLASSES ***

class Button {
    public:
        int8_t state = OFF;
        int8_t pin = D2;
        char* name = "A";

        Button ( int8_t buttonPin, char* buttonName) {
          pin = buttonPin;
          name = buttonName;
          pinMode (pin, INPUT_PULLUP);
        }

        void checkButton() {
            if (digitalRead( pin) == LOW) {
                if (state == OFF) {
                    delay(20); // wait to debounce button
                    if (digitalRead (pin) == LOW) {
                        log( String( " Pressed: ") + name);
                        topic = String( HOSTNAME) + String( "/") + String( name);
                        topic.toCharArray(loopTopic, MAX_TOPIC_LENGTH);
                        client.publish( loopTopic, "pressed");
                        state = ON;
                    }
                }
            } else {
                if (state == ON) {
                    delay(20); // wait to debounce button
                    if (digitalRead (pin) == HIGH) {
                        log( String( " Released: ") + name);
                        topic = String( HOSTNAME) + String( "/") + String( name);
                        topic.toCharArray(loopTopic, MAX_TOPIC_LENGTH);
                        client.publish( loopTopic, "released");
                        state = OFF;
                    }
                }
            }
        }
};

/*
need to know how to handle fonts
don't really want to pass name
  have extra flexibility possibility: normal, bold, italics, bold italics
  different famililies: monospaced, sans serif, serif

could just use a simple index, but for a family of devices, this matrix may
be sparse as some fonts are included and some are not.

The default font can be used with a magnification 1, 2, 4...

display.setFont(&FreeSans9pt7b);
display.setFont(&FreeSans12pt7b);
display.setFont(&FreeSans18pt7b);
display.setFont(&FreeSans24pt7b);

for 32x128 display:

basic 9pt baselines would be at 8, 18, and 28.
basic 12pt baselines could be 24 + 9 = 33... 12, 24, 31
18 + 12 = 30
18 + 9 = 27
in other words there is too much flexible to make the positioning generic


need a way to send:
  font family
  font style
  font size
  font x
  font y

if family, style or size not available, use default

*/


class Slot {
    public:
        char* name = "";
        String topic = "";
        String text = "";
        unsigned long expiry = NO_EXPIRATION;
        int8_t x = 0; // coordinates of slot on oled device
        int8_t y = 0;
        int8_t size = 1;
        int i;
        int len;

        Slot( char* slotName, int8_t slotX, int8_t slotY, int8_t slotSize) {
           name = slotName;
           topic = String( HOSTNAME) + String ("/") + String( name) + String( "/value");
           x = slotX;
           y = slotY;
           size = slotSize;
        };

        void updateSlot( String topicReceived, String payloadReceived, unsigned long now) {
	    if (topicReceived.compareTo( topic ) == 0) {
          if (payloadReceived.length() > 0) {
    	        text = payloadReceived;
	            log( String( "Set ") + name + String( " to: \"") + text + String ("\""));
	            expiry = now + SLOT_EXPIRY;
	            log( String( "Expiry set for ") + String( name) + String(" at ") + String( Time.format(expiry, "%I:%M:%S%p")));
          } else {
              text = "";
	            expiry = NO_EXPIRATION;
	            log( String( "Set ") + name + String( " to: None"));
          }
	    }
         };


        bool displaySlot( ) {
	    display.setTextSize( size);
	    display.setCursor( x, y);
	    text.toCharArray( stringBuffer, MAX_STRING_SIZE);
	    display.print( stringBuffer);
	    log( String( "Updating ") + name + String(": \"") + String(stringBuffer) + String( "\""));
        };

        bool checkSlotExpiration( unsigned long now) {
            // returns true if display needs updating
	    if ( expiry != NO_EXPIRATION && now > expiry) {
	        text = "";
                log( String("Expiring: ") + name);
	        expiry = NO_EXPIRATION;
	        return true;
	    } else {
                return false;
            }
        };
};


// *** LOCAL OBJECTS ***

// define button objects
//            (pin, "name"
Button buttonA (D2, "buttonA");
Button buttonB (D3, "buttonB");
Button buttonC (D4, "buttonC");


// define slot objects
//        ( "name",  x,  y, size)
Slot slot1( "slot1", 0,  0, 1);
Slot slot2( "slot2", 0,  8, 1);
Slot slot3( "slot3", 0, 16, 1);
Slot slot4( "slot4", 0, 24, 1);
Slot slot5( "slot5", 0,  0, 2);
Slot slot6( "slot6", 0, 11, 2);
Slot slot7( "slot7", 0, 22, 2);


//  *** FUNCTIONS ***

String yyyymmdd(unsigned long int now)  //format value as "yyyy/mm/dd"
{
   String year = String::format( "%i", Time.year(now));
   String month = String::format( "%i", Time.month(now));
   String day = String::format( "%i", Time.day(now));
   return year + "/" + month + "/" + day;
};

String hhmmss(unsigned long int now)  //format value as "hh:mm:ss"
{
   String hour = String(Time.hourFormat12(now));
   String minute = String::format("%02i",Time.minute(now));
   String second = String::format("%02i",Time.second(now));
   return hour + ":" + minute + ":" + second;
};


// for QoS2 MQTTPUBREL message.
// this messageid maybe have store list or array structure.
uint16_t qos2messageid = 0;

// MQTT receive message callback
void callback(char* topic, byte* payload, unsigned int length) {
    char lastPayload [MAX_PAYLOAD_SIZE+1];

    // payload must be copied and the terminating null added
    if (length > MAX_PAYLOAD_SIZE) {
      length = MAX_PAYLOAD_SIZE;
    }
    memcpy(lastPayload, payload, length);
    lastPayload[length] = NULL;
    currentTime = Time.now();
    String topicS = String( (const char*) topic);
    String payloadS = String( (const char*) lastPayload);
    payloadChanged = true;

    log( String( "Received message topic: \"" + topicS + "\" payload: \"" + payloadS + "\""));

    // update the slot according to the received topic with the received payload
    slot1.updateSlot( topicS, payloadS, currentTime);
    slot2.updateSlot( topicS, payloadS, currentTime);
    slot3.updateSlot( topicS, payloadS, currentTime);
    slot4.updateSlot( topicS, payloadS, currentTime);
    slot5.updateSlot( topicS, payloadS, currentTime);
    slot6.updateSlot( topicS, payloadS, currentTime);
    slot7.updateSlot( topicS, payloadS, currentTime);
}


// QOS ack callback.
// if application use QOS1 or QOS2, MQTT server sendback ack message id.
void qoscallback(unsigned int messageid) {
    log( String("Ack Message Id:" + messageid));

    if (messageid == qos2messageid) {
        log( String("Release QoS2 Message" + messageid));
        //client.publishRelease(qos2messageid);
    }
}


void log( String charArray) {
    // send the str to the serial output, if enabled
    Serial.println( hhmmss(ntptime.now()) + String(" ") + charArray);
}


void updateDisplay() {
    // control the display here
    // slots are filled by subscribe call backs
    // also need the general status/startup information
          // status/startup may be elsewhere because this should be in the loop

    display.clearDisplay(); // clear whatever is there.
    display.setTextColor(WHITE);

    display.invertDisplay(false);
    display.setTextColor(WHITE);

    if (slot5.text.length() >0) {
         slot5.displaySlot( );
    }
    if (slot6.text.length() >0) {
         slot6.displaySlot( );
    }
    if (slot7.text.length() >0) {
         slot7.displaySlot( );
    }
    if (slot5.text.length() == 0 && slot1.text.length() >0) {
         slot1.displaySlot( );
    }
    if (slot5.text.length() == 0 && slot6.text.length() == 0 && slot2.text.length() >0) {
         slot2.displaySlot( );
    }
    if (slot6.text.length() == 0 && slot7.text.length() == 0 && slot3.text.length() >0) {
         slot3.displaySlot( );
    }
    if (slot7.text.length() == 0 && slot4.text.length() >0) {
         slot4.displaySlot( );
    }

    display.display();
}

void updateDST (unsigned long int now) {
  // get time variables
  int month = Time.month(now);
  int day = Time.day(now);
  int weekday = Time.weekday(now);
  int hour = Time.hour(now);

  // daylight savings betwee
  //second Sunday in March
  // Sunday between the 8th and 14th
  //first Sunday in November
  // Sunday between 1st and 7th
  Time.endDST(); //assume standard time
  if ( ( month > 4) || \
       ( month == 4 &&  day >= 15) || \
       ( month == 4 && ( day >= 8 && day <= 14) && day - 7 > weekday) || \
       ( month == 4 && ( day >= 8 && day <= 14) && weekday == 1 && hour > 2) \
     && \
       ( month < 11 ) || \
       ( month == 11 && ( day < 7) ) || \
       ( month == 11 && ( day <= 7) && day > weekday ) || \
       ( month == 11 && ( day <= 7) && (weekday == 1) && hour < 2) ) {
    Time.beginDST ();
  }
}

void sendHeartbeat() {
      log( String( " Heart beat"));
      topic = String( HOSTNAME) + String( "/heatbeat");
      topic.toCharArray(loopTopic, MAX_TOPIC_LENGTH);
      client.publish( loopTopic, hhmmss(ntptime.now())); // may just want to send epoch
}



SYSTEM_MODE(MANUAL);

void setup() {
    Serial.begin(9600);
    log("starting");

    // set up network time synchronization
    ntptime.start();
    Time.zone(-5); //New York -5
    Time.setDSTOffset(1);

    pinMode (BUTTON_MANUAL, INPUT_PULLUP);

    // initialize display
    display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();

}


void loop() {
  bool displayNeedsUpdating = false;
  if (!WiFi.ready()) {
    WiFi.on();
    //WiFi.useDynamicIP();
    WiFi.connect();
    sendHeartbeat();
  }
  if (digitalRead (BUTTON_MANUAL) == LOW) {
    Particle.connect();
  }
  if (Particle.connected()) {
    Particle.process();
  }


    updateDST( ntptime.now());
    currentTime = Time.now();

    displayNeedsUpdating =  slot1.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot2.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot3.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot4.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot5.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot6.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot7.checkSlotExpiration( currentTime);

    // check heartbeat due
    if (millis() > heartBeatDue) {
      sendHeartbeat();
      heartBeatDue = millis() + HEARTBEAT_PERIOD;
    }

    // check buttons
    buttonA.checkButton();
    buttonB.checkButton();
    buttonC.checkButton();

    //display.println( yyyymmdd(ntptime.now()) + " " + \
    //


    if (payloadChanged | displayNeedsUpdating) {
      payloadChanged = false;
      log( String( "Updating display"));
      updateDisplay();
    }

    if (client.isConnected()) { // check on MQTT connection
        client.loop();  // logically this is where callback and qoscallback are invoked
    } else {
        // connect to the server
        log( String( " Attempting to connect to MQTT broker again"));
        client.connect(HOSTNAME);
        delay(1000);

        // subscribe to all slot values at once with wild card
        String topic = String( HOSTNAME) + String( "/+/value");
#define TOPIC_LEN 30
        char subscribeTopic[TOPIC_LEN];

        topic.toCharArray(subscribeTopic, TOPIC_LEN);
        client.subscribe( subscribeTopic);
    }
}
