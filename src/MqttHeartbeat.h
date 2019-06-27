/*
 * Project particle photon devices
 * Description:  provide an object to control MQTT heartbeat sending
 * Author: Kirk Carlson
 * Date: 8 Aug 2018- 2019
 */


// MQTT_HEARTBEAT_PERIOD is in milliseconds
#define MQTT_HEARTBEAT_PERIOD 10*60*1000

class MqttHeartbeat {
    public:
        unsigned long due = 0;

        // no contructor

        void check (unsigned long currentTime) {
            //check on LED heartbeat
            if (currentTime > due) {
                publish( "heartbeat", "");
                due = due + MQTT_HEARTBEAT_PERIOD; // no accumulated error
            }
        };

        void send( unsigned long currentTime) {
            publish( "heartbeat", "");
            due = currentTime + MQTT_HEARTBEAT_PERIOD; // reset timing
        };
};
