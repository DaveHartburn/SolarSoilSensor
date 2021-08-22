/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/home/dave/priv/arduino/Particle/SolarSoilSensor/src/SolarSoilSensor.ino"
// Minimalistic - build up slowly due to errors I was getting

void setup();
void loop();
void sendMessage(char *type, char *msg);
#line 3 "/home/dave/priv/arduino/Particle/SolarSoilSensor/src/SolarSoilSensor.ino"
#define BLEMODE   1     // zero to use the traditional Particle.publish
#define USESERIAL 1     // zero ignore the serial port. Will give battery savings
#define CALIBRATE 0     // 1 to enter soil calibration mode over serial (can't use particle function)

#if USESERIAL>0
SerialLogHandler logHandler(LOG_LEVEL_WARN, { {"app", LOG_LEVEL_ALL} });
#endif

void setup() {
    if(USESERIAL==1) {
        waitFor(Serial.isConnected, 3000);
        Log.info("Starting BLE Solar Soil Sensor with device ID "+System.deviceID()+"....");
    }
}

void loop() {
    sendMessage("Test", "In a loop");
    delay(5000);
}

void sendMessage(char *type, char *msg) {
    // Send data via Particle.publish, unless we are in BLE mode then output to serial (if active)
    if(BLEMODE==1) {
        // Currently dropping messages
    } else {
        Particle.publish(type, msg);
    }
    if(USESERIAL==1) {
        Log.info(msg);
    }
    #if USESERIAL==1
    Log.info("Really are using log serial");
    #endif

}