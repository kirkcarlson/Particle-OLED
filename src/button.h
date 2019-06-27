/************************************************************************
 * button class
 * Description:  read a button and optionally control associated LED
 * Author: Kirk Carlson
 * Date: 8 Aug 2018- 2019
 ***********************************************************************/

#ifndef _BUTTON
#define _BUTTON

// *** CONSTANTS ***
#define ON 1
#define OFF 0
#define DEBOUNCE 20 //milliseconds

// *** PROTOTYPES ***

void publish ( String topic, String payload);

// *** CLASSES ***

class Button {
    public:
        uint8_t state = OFF;
        uint8_t pin;
        uint8_t ledState = OFF;
        uint8_t pinLed = 0;
        unsigned long lastTime = 0;
        String topic;

//want to have an optional LED... Another prototype for this?
        Button ( uint8_t buttonPin, uint8_t ledPin, String buttonTopic) {
          // if no LED ledPin = buttonPin
          pin = buttonPin;
          pinLed = ledPin;
          topic = buttonTopic;
          pinMode (pin, INPUT_PULLUP);
          if (pin != pinLed) {
              pinMode (pinLed, OUTPUT);
          }
          pinMode (pin, INPUT_PULLUP);
        }

        void check( unsigned long epoch) {
            //epoch is epoch or number of milliseconds since starting

            if (lastTime == 0) { // no timer running
                if (epoch == 0) {
                    epoch = epoch + 1;
                }
                if (state == OFF && digitalRead( pin) == LOW) {
                    lastTime = epoch;
                } else if (state == ON && digitalRead( pin) == HIGH) {
                    lastTime = epoch;
                } // else no change indicated
            } else if (epoch - lastTime > DEBOUNCE) {
                lastTime = 0; //cancel timer regardless
                if (state == OFF && digitalRead( pin) == LOW) {
                    Log.info( topic + String( ": ON"));
                    publish( topic, "1");
                    state = ON;
                    if (pinLed != pin) {
                        digitalWrite(pinLed, HIGH);
                    }
                } else if (state == ON && digitalRead( pin) == HIGH) {
                    Log.info( topic + String( ": OFF"));
                    publish( topic, "0");
                    state = OFF;
                    if (pinLed != pin && ledState == OFF) {
                        digitalWrite(pinLed, LOW);
                    }
                } // else no change indicated
            }
        }

        bool stillOn() {
            return state != OFF;
        }


        uint8_t get_state() {
            return state;
        }


        void led( uint8_t ledCommand) {
            if (pinLed != pin) { // don't change here unless associated
                if (ledCommand != OFF) { // allow for other on commands like
                                        // slow flash, fast flash, wink, blink
                    digitalWrite(pinLed, HIGH);
                    ledState = ON;
                } else {
                    ledState = OFF;
                    if (state == OFF) { // if button ON, let LED stay on
                                        // until button OFF
                        digitalWrite(pinLed, LOW);
                    }
                }
            }
        }
};
#endif// _BUTTON
