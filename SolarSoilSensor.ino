// Minimalistic - build up slowly due to errors I was getting

#define BLEMODE   0     // zero to use the traditional Particle.publish
#define USESERIAL 1     // zero ignore the serial port. Will give battery savings
#define CALIBRATE 0     // 1 to enter soil calibration mode over serial (can't use particle function)

#if USESERIAL==1
SerialLogHandler logHandler(LOG_LEVEL_WARN, { {"app", LOG_LEVEL_ALL} });
#endif

// Include device IDs. See comments
#include <deviceIDs.h>




void setup() {
    if(USESERIAL==1) {
        waitFor(Serial.isConnected, 3000);
        Log.info("Starting BLE Solar Soil Sensor with device ID "+System.deviceID()+"....");
    }
}

void loop() {
    sendMessage("Test", "In a loop");
    char s[100];
    sprintf(s, "device id 0 is %s, %s", deviceIDs[0].id.c_str(), deviceIDs[0].name.c_str());
    sendMessage("Test", s);
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
}