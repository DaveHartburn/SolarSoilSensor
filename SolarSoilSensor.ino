/* SolarSoilSensor - Dave Hartburn, April 2020
 *
 * Particle Xenon based solar powered soil sensor. Intended to be based in the garden and run for a few
 * months reporting soil moisture content with environmental details.
 * 
 * BLE version - rather than Particle.publish, particle cloud functions are disabled and
 * data sent as a JSON string over BLE. Due to mesh retirement.
 *
 * Hardware:
 *  Particle Xenon (though Argon may also be an option after the mesh is discontinued)
 *  INA219 current monitoring board
 *  BME280 environmental sensor
 *  Solar cell (85x115 or 80x55 depending on model)
 *  L7805cv voltage regulator
 *  Capacitive Soil Moisture Sensor v1.2
 *  2000mah battery (also have 18650s)
 *
 * Brief wiring description:
 * BME280 and INA219, both I2C devices connected to power, SDA (D0), SCL (D1)
 * INA219 Vin+ (power in) connects to the solar panel via a IN5819 diode. This allows us to monitor the total generated voltage rather than the regulated
 *   Vin- (power out) connects through the V7805cv then to VUSB.
 *   2 x 10uF capacitors between the pins on the V7805cv
 * Soil mositure, to power and A2
 *
 * A twin dip switch runs on ports D4 and D5 (other end to GND) to configure test mode
 * (short delay, no sleep) (switch 1 to ON) and sleep mode (switch 1 to OFF) and a delay
 * time (switch 2 to ON for short delay, OFF for long). Config status only checked on
 * boot.
 * Switch   Status  digRead     slMode/delay
 *   1        ON      LOW         0 - wait
 *   1        OFF     HIGH        1 - sleep
 *   2        ON      LOW         Short
 *   2        OFF     HIGH        Long
 *   
 * Both off: Normal sleep mode with the longer delay (also default if switch fails)
 * Both on: Dev/test mode with short delay
 * 
 * To Do
 * -----
 * Sensible sleeping without BLE on
 * Enable calibration without cloud
*/

#define BLEMODE   1     // zero to use the traditional Particle.publish
#define USESERIAL 0     // zero ignore the serial port. Will give battery savings
#define CALIBRATE 0     // 1 to enter soil calibration mode over serial (can't use particle function)
                        // Not yet implemented

#if BLEMODE==1
    // Do not use mesh
    SYSTEM_MODE(MANUAL)
#endif

// Although not used in normal cloud mode, we still define the BLE constants so the rest of the code compiles
#define CONNECT_TIMEOUT 45     // How long to time out waiting for a connection in seconds
#define SEND_ATTEMPTS 5       // How many times to try and send the data
#define SEND_INTERVAL 5

// Define BLE services
const BleUuid homeBLEservice("d1bedc42-0328-11ec-9a03-0242ac130003");   // UUID to use for all home BLE->Cloud services
// JSON string to combine all values
BleCharacteristic jsonOutChar("JSON", BleCharacteristicProperty::NOTIFY, BleUuid("d692e9d8-1c92-4b3d-b387-801247613d66"), homeBLEservice);


// Enable serial
#if USESERIAL==1
    SerialLogHandler logHandler(LOG_LEVEL_WARN, { {"app", LOG_LEVEL_ALL} });
#endif

// Include device IDs. See comments
#include <deviceIDs.h>
String devName="Unk-Xen";  // Default device name, max 8 chars. Will need to change to deploy elsewhere

#define CONFPIN_SLEEP   D4
#define CONFPIN_DELAY   D5
#define DELAY_SHORT     5000    // Short delay - 5 seconds
#define DELAY_LONG      15000   // 15 seconds
#define SLEEP_SHORT     60     // Sleep for 1 minutes
#define SLEEP_LONG      300     // Sleep for 5 minutes

// Init INA219 sensor
#include <adafruit-ina219.h>
Adafruit_INA219 sensor219;

// Init BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
// Set up BME
#define BME_ADDRESS 0x76    // Use i2c_scanner to determine address

#define SOILPIN A2

// Fix to allow antenna selection to work. Conflict in EXTERNAL being used twice
#undef EXTERNAL

double batVolts=0.0;
float pres, temp, humid;
String jsonOut;
int soilValue;
int soilPC=0;         // Soil percentage
int waitTime;       // How long to wait between readings
int slMode;          // Sleep mode: 1 sleep, 2 just wait
int antenna;        // 0 for internal - default, 1 for external

// Sensor calibration values
int soilDry = 4095;        // Soil dry value (used for calibrated readout)
int soilWet = 0;        // 100% wet, i.e. sensor in water

// Function prototypes
int setSoilDry(int i);
int setSoilWet(int i);
// Cloud functions to set calibration - must use string
int setSoilDryStr(String s);
int setSoilWetStr(String s);
// One to set the antenna
int setAntennaStr(String s);

// Where to store in EEPROM
int dryAddr = 0;    // (int is only 2 bytes)
int wetAddr = 4;    // Giving extra space for future expansion
int antAddr = 8;    // Antenna type

void setup() {
    if(USESERIAL==1) {
        waitFor(Serial.isConnected, 3000);
    }
    // Send initial message
    sendMessage("Start", "Soil sensor awake with device ID "+System.deviceID());

    // Start I2C
    sensor219.begin();
    Wire.begin();

    // Work out name
    int l=sizeof(deviceIDs)/sizeof(devID);
    for(int i=0; i<l; i++) {
        if(deviceIDs[i].id==System.deviceID()) {
            devName=deviceIDs[i].name;
            sendMessage("Start", "Set device name to "+devName);
        }
    }

    // Check power and charging status
    pinMode(PWR, INPUT);
    pinMode(CHG, INPUT);

    if(bme.begin(BME_ADDRESS)) {
        sendMessage("Start", "BME Sensor found");
    } else {
        sendMessage("Start", "Warning: No BME sensor found");
    }
    pinMode(SOILPIN,INPUT);
    
    // Init config pins
    pinMode(CONFPIN_SLEEP, INPUT_PULLUP);
    pinMode(CONFPIN_DELAY, INPUT_PULLUP);
    
    int pinSleep = digitalRead(CONFPIN_SLEEP);
    int pinDelay = digitalRead(CONFPIN_DELAY);
    
    char msg[100];
    if(pinSleep == HIGH) {
        // switch 1 is off, use sleep mode
        slMode=1;
        if(pinDelay == HIGH) {
            // Switch is off, use long delay
            waitTime = SLEEP_LONG;
        } else {
            // Switch is on, use short delay
            waitTime = SLEEP_SHORT;
        }
    } else {
        // Switch 1 is off, use wait mode
        slMode=0;
        if(pinDelay == HIGH) {
            // Switch is off, use long delay
            waitTime = DELAY_LONG;
        } else {
            // Switch is on, use short delay
            waitTime = DELAY_SHORT;
        }
        
    }
    sendMessage("Start", String::format("Sleep mode = %d, waitTime = %d", slMode, waitTime));

    // Register cloud variables if not in BLE mode
    #if BLEMODE==0
        Particle.variable("soilPC", soilPC);
        Particle.variable("batVolts", batVolts);
        
        // Register calibration functions
        Particle.function("setSoilWet", setSoilWetStr);
        Particle.function("setSoilDry", setSoilDryStr);
        Particle.function("setAntenna", setAntennaStr);
    #else
        // Add characteristic to BLE 
        BLE.addCharacteristic(jsonOutChar);
    #endif

    // Get wet and dry values from EEPROM
    int sd, sw;
    EEPROM.get(dryAddr, sd);
    EEPROM.get(wetAddr, sw);
    if(sd < 0 || sd > 4095) {
        // EEPROM default value, set to defaults in header
        setSoilDry(soilDry);
    } else {
        soilDry=sd;
    }
    if(sw < 0 || sw > 4095) {
        // EEPROM default value, set to defaults in header
        setSoilWet(soilWet);
    } else {
        soilWet=sw;
    }
    
    // Which antenna to use?
    EEPROM.get(antAddr, antenna);
    setAntenna(antenna);    

    sendMessage("Start", String::format("soilDry=%d, soilWet=%d, antenna=%d", soilDry, soilWet, antenna));
}

void loop() {
    // Read power details
    float busVoltage = 0;
    float current = 0; // Measure in milli amps
    float power = 0;

    busVoltage = sensor219.getBusVoltage_V();
    current = sensor219.getCurrent_mA();
    power = busVoltage * (current/1000); // Calculate the Power    
    
    int inputPwr, inputChg, charge, onBatteryPower;
    
    batVolts = analogRead(BATT) * 0.0011224;

    inputPwr=digitalRead(PWR);
    inputChg=digitalRead(CHG);

    charge = (inputPwr && !inputChg);  // 0 = Not Charging
    onBatteryPower = (!inputPwr);              // 0 = Not on Battery Power

    // Read soil sensor value
    int s = analogRead(SOILPIN);
    
    // Read BME environmental data
    temp = bme.readTemperature();
    humid = bme.readHumidity();
    pres = bme.readPressure() / 100.0F;
    
    // Debug output (comment out) to ensure we are setting soil calibration from cloud
    //Particle.publish("debug", String::format("soilDry %d, soilWet %d", soilDry, soilWet), PRIVATE);
    
    // Calculate moisture as a percentage
    // Sensor reads high value = dry, low value = wet. Makes more sense if 100% moisture
    // is very wet and 0 is very dry, so invert.
    int r = soilDry-soilWet;        // Range of values
    soilPC = 100-((s-soilWet)*100/r);

    // Output data as one large JSON
    jsonOut=String::format("{'battery': %0.2f, 'busVoltage': %0.2f, 'soilMoisture': %d, 'soilPC': %d, 'humidity': %0.2f, 'temperature': %0.2f, 'pressure': %0.2f}",
        batVolts, busVoltage, s, soilPC, humid, temp, pres);
    sendMessage("SoilData", jsonOut);
    
    if(slMode==1) {
        // Sleep mode, use deep sleep function
        sendMessage("System", "Going to deep sleep");
        // Make absolutely sure BLE is off, on but sleeping can cause a gateway crash
        if(BLEMODE==1) {
            BLE.off();
        }
        System.sleep(D8, RISING, waitTime);
    } else {
        // Just wait mode, stay alive
        sendMessage("System", "Waiting, not deep sleeping...");
        delay(waitTime);
    }
    sendMessage("System", "Awake again");
}

void sendMessage( String type, String msg) {
    // Send data via Particle.publish, unless we are in BLE mode then output to serial (if active)
    if(BLEMODE==1) {
        if(type=="SoilData") {
            // This is data we want to send
            // We don't just send the JSON data, it needs to be nested as a payload.
            String sendStr=String::format("{\"sensorName\": \"%s\", \"apiKey\": \"unset\", \"payload\":\"%s\"}",
      devName.c_str(), msg.c_str());
            sendMessage("", "Soil data received, BLE sending "+sendStr);

            // Don't restart BLE if already connected
            if(!BLE.connected()) {
                // Turn on BLE
                BLE.on();
                // Set up BLE data
                BleAddress badd=BLE.address();
                sendMessage("", String::format("Starting BLE service with address %02x:%02x:%02x:%02x:%02x:%02x", badd[5], badd[4], badd[3], badd[2], badd[1], badd[0]));

                BleAdvertisingData bleData;
                // Add data to the advertising object
                bleData.appendLocalName(devName);  // Max of 8 characters with 128-bit UUID
                bleData.appendServiceUUID(homeBLEservice);
                BLE.advertise(&bleData);
            }


            // Wait while we send the message via BLE (if needed)
            unsigned long timeout=millis()+CONNECT_TIMEOUT*1000;
            boolean quitLoop=false;
            unsigned long nextSend;
            boolean hasConnected=false;   // Track if we were ever connected
            int sendAttempts=0;            // Track how many times we try to send data

            while(quitLoop==false) {
                //Log.info("Inner loop");
                if(BLE.connected()) {
                    // We have made a connection, assume this is to the gateway
                    if(hasConnected==false) {
                        // First time this has connected
                    hasConnected=true;
                        nextSend=millis()+3000; // Give 3s for the connection to be established before sending
                        sendMessage("log", "BLE connection made");
                    }
                    if(millis()>nextSend) {
                        // Try multiple time to send value until disconnected or limit reached
                        sendMessage("log", "Sending data");
                        jsonOutChar.setValue(sendStr);
                        // Increase counter and set next time
                        sendAttempts++;
                        nextSend=millis()+SEND_INTERVAL*1000;
                        if(sendAttempts>SEND_ATTEMPTS) {
                            sendMessage("log", "Reached maximum send attemps");
                            quitLoop=true;
                        }
                    }
                } else {
                    // Not BLE connected
                    // Have we previously connected?
                    if(hasConnected==true) {
                        // Yes, gateway has dropped the connection, drop into deepsleep
                        sendMessage("log", "Central device disconnected us");
                        quitLoop=true;
                    } else {
                        // Never connected
                        if(millis()>timeout) {
                            sendMessage("log", "Timeout");
                            quitLoop=true;
                        }
                    }
                }
            }           
        }
        // Anything other than soil data is dropped
        // End of BLE mode check
    } else {
        // Not BLE, send with particle
        Particle.publish(type, msg);
    }
    if(USESERIAL==1) {
        Log.info(msg);
    }
}

// Calibration functions - called from cloud
int setSoilWetStr(String s) {
    int i;
    i = s.toInt();
    setSoilWet(i);
    return 0;
}
int setSoilDryStr(String s) {
    int i;
    i = s.toInt();
    setSoilDry(i);
    return 0;
}
int setAntennaStr(String s) {
    int i,j;
    i = s.toInt();
    j = setAntenna(i);
    if(i==0) {
        sendMessage("Message", "Changed to internal antenna");
    } else {
        sendMessage("Message", "Changed to external antenna");
    }
    return j;
}
// Actual functions after conversion
int setSoilWet(int i) {
    soilWet=i;
    EEPROM.put(wetAddr, i);
    sendMessage("Message", "Soil wet value set");
    return 0;
}
int setSoilDry(int i) {
    soilDry=i;
    EEPROM.put(dryAddr, i);
    sendMessage("Message", "Soil dry value set");
    return 0;
}
int setAntenna(int i) {
    // Save to EEPROM
    EEPROM.put(antAddr, i);
    #if BLEMODE==0
        if(i==0) {
            Mesh.selectAntenna(MeshAntennaType::INTERNAL);
        } else {
            Mesh.selectAntenna(MeshAntennaType::EXTERNAL);
        }
    #else
        if(i==0) {
           BLE.selectAntenna(BleAntennaType::INTERNAL);
        } else {   
           BLE.selectAntenna(BleAntennaType::EXTERNAL);
        }
    #endif
    return i;
}