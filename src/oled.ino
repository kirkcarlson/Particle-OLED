/*
 * Project oled
 * Description:  control an OLED display with generalized MQTT data
 * Author: Kirk Carlson
 * Date: 8 Aug 2018- 2019
 */

/*********************************************************************
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

// *** INCLUDES ***

#include <MQTT.h>
#include <NtpTime.h>
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
//#include "FreeSans9pt7b.h"
//#include "FreeSans12pt7b.h"
//#include "FreeSans18pt7b.h"
//#include "FreeSans24pt7b.h"
#include "addresses.h"

// check that the source code has been modified
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

 
// *** HARDWARE DEFINES ***
#define SSD1306_I2C_ADDRESS	0x3C
// SDA			D0
// SCL			D1
#define BUTTON_A	D2
#define BUTTON_B	D3
#define BUTTON_C	D4
#define OLED_RESET	D5
#define BUTTON_MANUAL	D6

// *** CONSTANTS ***
#define ON 1
#define OFF 0
#define MAX_PAYLOAD_SIZE 100
// SLOT_EXPIRY is in seconds
#define SLOT_EXPIRY 5*60
// HEARTBEAT_PERIOD is in milliseconds
#define HEARTBEAT_PERIOD 10*60*1000
#define NO_EXPIRATION 0
// KEEP_ALIVE is in seconds
#define KEEP_ALIVE 60
#define MAX_TOPIC_LENGTH 30
#define MAX_STRING_SIZE 32

#define SOURCE_MQTT 0
#define SOURCE_TIME 1
#define SOURCE_IP 2
#define SOURCE_STATUS 3
#define SOURCE_PING 4

#define COLOR_NORMAL 0
#define COLOR_INVERSE 1

byte google [] = { 8,8,8,8 };


// *** GLOBALS ***
// yes this is bad, a few globals. Just to save memory
// there are used only as temporary variables within functions
// but they are common
String topic;
char loopTopic[ MAX_TOPIC_LENGTH + 1];
String status;
char stringBuffer[ MAX_STRING_SIZE + 1];
unsigned long  currentTime = 0;
unsigned long  heartBeatDue = 0;
unsigned long  timeUpdateDue = 0;
int8_t pingCount = 0;
bool displayNeedsUpdating = false;

// *** INTERFACE OBJECTS ***

NtpTime ntptime;
//MQTT client( server, 1883, callback);
//need initialization verions that include keep alive timer values
MQTT client( server, 1883, KEEP_ALIVE, callback);

//Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire, OLED_RESET);
Adafruit_SSD1306 display (OLED_RESET);


//  *** CLASSES ***

class Button {
    public:
        int8_t state = OFF;
        int8_t pin = BUTTON_A;
        String name = "A";

        Button ( int8_t buttonPin, String buttonName) {
          pin = buttonPin;
          name = buttonName;
          pinMode (pin, INPUT_PULLUP);
        }

        void checkButton() {
            String topic;

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


class Slot {
    public:
        String name = "";
        String topic = "";
        String text = "";
        unsigned long expiry = NO_EXPIRATION;
        int8_t x = 0; // coordinates of slot baseline on OLED device
        int8_t y = 0;
        int8_t size = 1;
        int8_t color = COLOR_NORMAL;
        int8_t source = SOURCE_MQTT;
        int i;
        int len;

        Slot( String slotName, int8_t slotX, int8_t slotY, int8_t slotSize) {
           name = slotName;
           topic = String( HOSTNAME) + String ("/") + String( name) + String( "/value");
           x = slotX;
           y = slotY;
           size = slotSize;
        };

        void updateSlot( String slotString) {
            if (slotString.length() > 0) {
                text = slotString;
                log( String( "Set ") + name + String( " to: \"") + slotString + String ("\""));
                expiry = currentTime + SLOT_EXPIRY;
                log( String( "Expiry set for ") + String( name) + String(" at ") + String( Time.format(expiry, "%I:%M:%S%p")));
            } else {
                text = "";
                expiry = NO_EXPIRATION;
                log( String( "Set ") + name + String( " to: None"));
            }
        };


        void displaySlot( ) {
            String scratch;

            display.setTextSize( size);
            display.setCursor( x, y);
            if (color == 0) {
                display.invertDisplay(false);
                display.setTextColor(WHITE);
            } else {
                display.invertDisplay(true);
                display.setTextColor(BLACK);
            }
            switch (source) {
            case SOURCE_TIME:
                hhmmss(ntptime.now()).toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            case SOURCE_IP:
                scratch = String (WiFi.localIP());
                //String (localIP[0] + "." + localIP[1] + "." + localIP[2] + "." +
                //    localIP[3]).toCharArray( stringBuffer, MAX_STRING_SIZE);
                scratch.toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            case SOURCE_STATUS:
                status.toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            case SOURCE_MQTT:
                text.toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            case SOURCE_PING:
                scratch = String( "Ping: ") + String( pingCount);
                scratch.toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            default:
                text.toCharArray( stringBuffer, MAX_STRING_SIZE);
                break;
            }
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
//            (pin, "name")
Button buttonA (BUTTON_A, "buttonA");
Button buttonB (BUTTON_B, "buttonB");
Button buttonC (BUTTON_C, "buttonC");


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

void setSlot( int8_t slotNumber, int8_t source )
//set the source for a given slotNumber
{
    switch (slotNumber) {
    case 1:
        slot1.source = source;
        break;
    case 2:
        slot2.source = source;
        break;
    case 3:
        slot3.source = source;
        break;
    case 4:
        slot4.source = source;
        break;
    case 5:
        slot5.source = source;
        break;
    case 6:
        slot6.source = source;
        break;
    case 7:
        slot7.source = source;
        break;
    default:
        break;
    }
}

void colorSlot( int8_t slotNumber, int8_t color )
//set the color (normal, inverse) for a given slotNumber
{
    switch (slotNumber) {
    case 1:
        slot1.color = color;
        break;
    case 2:
        slot2.color = color;
        break;
    case 3:
        slot3.color = color;
        break;
    case 4:
        slot4.color = color;
        break;
    case 5:
        slot5.color = color;
        break;
    case 6:
        slot6.color = color;
        break;
    case 7:
        slot7.color = color;
        break;
    default:
        break;
    }
}


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
    lastPayload[length] = '\0';
    currentTime = Time.now();
    String topicS = String( (const char*) topic);
    String payloadS = String( (const char*) lastPayload);
    displayNeedsUpdating = true;

    log( String( "Received message topic: \"" + topicS + "\" payload: \"" + payloadS + "\""));

/*
want to have a switch case statement here to handle various options
node/slot1/value	payload = text
node/slot2/value	payload = text
node/slot3/value	payload = text
node/slot4/value	payload = text
node/slot5/value	payload = text
node/slot6/value	payload = text
node/slot7/value	payload = text
node/timeSlot	payload = textual slot number
node/ipSlot	payload = textual slot number
node/statSlot	payload = textual slot number
node/mqttSlot	payload = textual slot number
node/invSlot	payload = textual slot number
node/normSlot	payload = textual slot number

*/
    // update the slot according to the received topic with the received payload


    if (topicS.compareTo( String( HOSTNAME) + String( "/timeSlot")) == 0) {
        setSlot( payloadS.toInt(), SOURCE_TIME);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/ipSlot")) == 0) {
        setSlot( payloadS.toInt(), SOURCE_IP);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/statSlot")) == 0) {
        setSlot( payloadS.toInt(), SOURCE_STATUS);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/mqttSlot")) == 0) {
        setSlot( payloadS.toInt(), SOURCE_MQTT);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/pingSlot")) == 0) {
        setSlot( payloadS.toInt(), SOURCE_PING);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/invSlot")) == 0) {
        colorSlot( payloadS.toInt(), COLOR_INVERSE);
    } else if (topicS.compareTo( String( HOSTNAME) + String( "/normSlot")) == 0) {
        colorSlot( payloadS.toInt(), COLOR_NORMAL);
    } else if (topicS.compareTo( slot1.topic) == 0) {
        slot1.updateSlot( payloadS);
    } else if (topicS.compareTo( slot2.topic) == 0) {
        slot2.updateSlot( payloadS);
    } else if (topicS.compareTo( slot3.topic) == 0) {
        slot3.updateSlot( payloadS);
    } else if (topicS.compareTo( slot4.topic) == 0) {
        slot4.updateSlot( payloadS);
    } else if (topicS.compareTo( slot5.topic) == 0) {
        slot5.updateSlot( payloadS);
    } else if (topicS.compareTo( slot6.topic) == 0) {
        slot6.updateSlot( payloadS);
    } else if (topicS.compareTo( slot7.topic) == 0) {
        slot7.updateSlot( payloadS);
    }
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


void log( String message) {
    // send the message to the serial output, if enabled
    Serial.println( hhmmss(ntptime.now()) + String(": ") + message);
}


void updateDisplay() {
    // control the display here
    // slots are filled by subscribe call backs or local status data

    log( String( "Updating display"));
    display.clearDisplay(); // clear whatever is there.

    if (slot5.text.length() >0) {
        slot5.displaySlot( );
    }
    if (slot6.text.length() >0) {
        slot6.displaySlot( );
    }
    if (slot7.text.length() >0) {
        slot7.displaySlot( );
    }
    if (slot1.source != SOURCE_MQTT || (
        slot5.text.length() == 0 &&
        slot1.text.length() >0) ) {
            slot1.displaySlot( );
    }
    if (slot2.source != SOURCE_MQTT || (
        slot5.text.length() == 0 &&
        slot6.text.length() == 0 &&
        slot2.text.length() >0) ) {
            slot2.displaySlot( );
    }
    if (slot3.source != SOURCE_MQTT || (
        slot6.text.length() == 0 &&
        slot7.text.length() == 0 &&
        slot3.text.length() >0) ) {
            slot3.displaySlot( );
    }
    if (slot4.source != SOURCE_MQTT || (
        slot7.text.length() == 0 &&
        slot4.text.length() >0) ) {
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

  // daylight savings between
  //second Sunday in March
  // Sunday between the 8th and 14th
  //first Sunday in November
  // Sunday between 1st and 7th
  Time.endDST(); //assume standard time
  if ( (
         ( month > 4) || \
         ( month == 3 &&  day >= 15) || \
         ( month == 3 && ( day >= 8 && day <= 14) && day - 7 > weekday) || \
         ( month == 3 && ( day >= 8 && day <= 14) && weekday == 1 && hour > 2) \
       )
     && \
       (
         ( month < 11 ) || \
         ( month == 11 && ( day < 7) ) || \
         ( month == 11 && ( day <= 7) && day > weekday ) || \
         ( month == 11 && ( day <= 7) && (weekday == 1) && hour < 2)
       )
    ) {
    Time.beginDST ();
  }
}

void sendHeartbeat() {
    String topic;

    log( String( " Heart beat"));
    topic = String( HOSTNAME) + String( "/heatbeat");
    topic.toCharArray(loopTopic, MAX_TOPIC_LENGTH);
    client.publish( loopTopic, hhmmss(ntptime.now())); // may just want to send epoch
}



SYSTEM_MODE(MANUAL);

void setup() {
    status = "Starting up";
    Serial.begin(9600);
    log("starting");

    // set up network time synchronization
    ntptime.start();
    Time.zone(-5); //New York -5
    Time.setDSTOffset(1);

    pinMode (BUTTON_MANUAL, INPUT_PULLUP);

    // initialize display
    //display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, true, true);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.display(); // Adafruit flash screen
    delay(1000);
    display.clearDisplay();
    display.display();
    displayNeedsUpdating = false;


    // initialize local slot control
    setSlot(1, SOURCE_STATUS);
    setSlot(2, SOURCE_PING);
    setSlot(3, SOURCE_IP);
    setSlot(4, SOURCE_TIME);
    timeUpdateDue = millis(); // OK to be due out of the gate
}


void subscribe (String topic) {
#define TOPIC_LEN 30
        char subscribeTopic[TOPIC_LEN];

        topic.toCharArray(subscribeTopic, TOPIC_LEN);
        client.subscribe( subscribeTopic);
}


void loop() {
  if (!WiFi.ready()) {
    status = "WiFi connecting";
    WiFi.on();
    //WiFi.useDynamicIP();
    WiFi.connect();
    sendHeartbeat();
    pingCount = WiFi.ping( google ); //hopefully this will time out eventually
  }
  status = "WiFi up";
  if (digitalRead (BUTTON_MANUAL) == LOW) {
    Particle.connect();
  }
  if (Particle.connected()) {
    Particle.process();
  }

    if (millis() > timeUpdateDue) {
        timeUpdateDue = timeUpdateDue + 1000; // don't accumulate error
        updateDST( ntptime.now());
        displayNeedsUpdating = true;
    }
    currentTime = Time.now();

    displayNeedsUpdating |= slot1.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot2.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot3.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot4.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot5.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot6.checkSlotExpiration( currentTime);
    displayNeedsUpdating |= slot7.checkSlotExpiration( currentTime);

    // check heartbeat due
    if (millis() > heartBeatDue) {
      sendHeartbeat();
      pingCount = WiFi.ping( google ); //hopefully this will time out eventually
      heartBeatDue = millis() + HEARTBEAT_PERIOD;
    }

    // check buttons
    buttonA.checkButton();
    buttonB.checkButton();
    buttonC.checkButton();

    // check update display
    if ( displayNeedsUpdating) {
      displayNeedsUpdating = false;
      updateDisplay();
    }

    if (client.isConnected()) { // check on MQTT connection
        status = "MQTT up";
        client.loop();  // logically this is where callback and qoscallback are invoked
    } else {
        // connect to the server
        status = "MQTT connecting";
        log( String( " Attempting to connect to MQTT broker again"));
        client.connect(HOSTNAME);
        delay(1000);

        // subscribe to all slot values at once with wild card
        subscribe( String ( HOSTNAME) + String( "/+/value"));
        // subscribe to all messages addressed to node
        //subscribe( String( HOSTNAME) + String( "/#"));

        subscribe( String ( HOSTNAME) + String( "/timeSlot"));
        subscribe( String ( HOSTNAME) + String( "/ipSlot"));
        subscribe( String ( HOSTNAME) + String( "/statSlot"));
        subscribe( String ( HOSTNAME) + String( "/mqttSlot"));
        subscribe( String ( HOSTNAME) + String( "/pingSlot"));
        subscribe( String ( HOSTNAME) + String( "/invSlot"));
        subscribe( String ( HOSTNAME) + String( "/normSlot"));
    }
}
