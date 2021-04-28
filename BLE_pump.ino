#include <ArduinoBLE.h>
#include <AccelStepper.h>

/// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
// Stepper 1
#define enPin 12
#define ms1Pin 11 
#define ms2Pin 10    
#define stepPin 9   
#define dirPin 8     
#define motorInterfaceType 1
//spreadPin controls modi between 0=stealthChop and 1=spreadCycle of all connected stepper drivers (TMC220x)
#define spreadPin 2  

/// Create instances of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

/// Define global variables
float steps_per_second = 0;
float total_steps = 0;


//BLEPeripheral blePeripheral;  // BLE Peripheral Device (the board you're programming)
BLEService pumpService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE LED Service

// BLE Stepper Steps Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic stepsCharacteristic("00002a00-0000-1000-8000-00805f9b34fb", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic speedCharacteristic("1205d6e3-1a79-4b03-aa02-ce0119f79263", BLERead | BLEWrite);

void setup() {
  Serial.begin(19200);
  // set LED pin to output mode
  pinMode(LED_BUILTIN, OUTPUT);
  if (!BLE.begin()) 
  {
  Serial.println("starting BLE failed!");
  while (1);
  }

  // set advertised local name
  BLE.setLocalName("BLE-Pump");

  // add service and characteristic:
  BLE.setAdvertisedService(pumpService);
  pumpService.addCharacteristic(stepsCharacteristic);
  pumpService.addCharacteristic(speedCharacteristic);
  BLE.addService(pumpService);

  // begin advertising BLE service:
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");

  // set pin modes
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH); //deactivate driver (LOW active)
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW); //LOW or HIGH
  pinMode(stepPin, OUTPUT);
  digitalWrite(stepPin, LOW);

  digitalWrite(enPin, LOW); //activate driver
  
  // set the maximum speed in steps per second:
  stepper.setMaxSpeed(2000);

  // set Current Position
  stepper.setCurrentPosition(0); 
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    String data_steps = "";
    int char_counter_steps = 0;

    String data_speed = "";
    int char_counter_speed = 0;
    
    // while the central is still connected to peripheral:
    digitalWrite(LED_BUILTIN, HIGH); 
    while (central.connected()) {
      
      if (stepsCharacteristic.written()) {
        int value = stepsCharacteristic.value() - '0';
        data_steps.concat(value);
        char_counter_steps++;
      
        if (char_counter_steps == 10) {
          total_steps = data_steps.toFloat();
          Serial.println("Steps: " + String(total_steps));
          data_steps = "";  
          char_counter_steps = 0;     
        }
      }
      
      if (speedCharacteristic.written()) {
        int value = speedCharacteristic.value() - '0';
        data_speed.concat(value);
        char_counter_speed++;
      
        if (char_counter_speed == 10) {
          steps_per_second = data_speed.toFloat();
          Serial.println("Speed: " + String(steps_per_second));
          data_speed = "";  
          char_counter_speed = 0;     
        }
      }
    }
    // reset stepper position to zero
    stepper.setCurrentPosition(0);
    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, LOW); 
  }

  //Move stepper as long not all steps have been perfomed
  if (stepper.currentPosition() != total_steps) {
    stepper.setSpeed(steps_per_second);
    stepper.runSpeed();
  }
  
}
