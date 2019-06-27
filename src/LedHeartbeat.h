/*
 * Project particle photon devices
 * Description:  provide an object to control LED heartbeat flashing
 * Author: Kirk Carlson
 * Date: 8 Aug 2018- 2019
 */


// LED_HEARTBEAT_PERIOD is half cycle in milliseconds
#define LED_HEARTBEAT_PERIOD 250
class LedHeartbeat {
    public:
        char port;
        unsigned long due;
        boolean state = false;

        LedHeartbeat(char iport) {
            port = iport;
            due = 0;
            state = false;
        };

        void check (unsigned long currentTime) {
            //check on LED heartbeat
            if (currentTime > due) {
                state = !state;
                if (state) {
                    digitalWrite( port, HIGH);
                } else {
                    digitalWrite( port, LOW);
                }
                due = due + LED_HEARTBEAT_PERIOD;
            }
        };
};


