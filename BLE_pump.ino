#include <ArduinoBLE.h>
#include <AccelStepper.h>

/// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
// Stepper pump
#define enPin_pump 12
#define ms1Pin_pump 11 
#define ms2Pin_pump 10    
#define stepPin_pump 9   
#define dirPin_pump 8     
#define motorInterfaceType_pump 1

/// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
// Stepper pump
#define enPin_fractionArm 7
#define ms1Pin_fractionArm 6 
#define ms2Pin_fractionArm 5    
#define stepPin_fractionArm 4   
#define dirPin_fractionArm 3     
#define motorInterfaceType_fractionArm 1

//spreadPin controls modi between 0=stealthChop and 1=spreadCycle of all connected stepper drivers (TMC220x)
#define spreadPin 2  


/// Create instances of the AccelStepper class:
AccelStepper stepper_pump = AccelStepper(motorInterfaceType_pump, stepPin_pump, dirPin_pump);
AccelStepper stepper_fractionArm = AccelStepper(motorInterfaceType_fractionArm, stepPin_fractionArm, dirPin_fractionArm);

/// Define global variables
float steps_per_second_pump = 0;
float steps_per_second_fractionArm = 0;
float total_steps_pump = 0;

int mode_index = 0;

float fractionArm_switchSteps = 89;   //steps needed for Arm to switch from ONE fraction to the next one [ empirical constant]

float target_steps_fractionArm = 0;
float steps_per_pump_fraction = 0;
float target_steps_pump = 0;  

//BLEPeripheral blePeripheral;  // BLE Peripheral Device (the board you're programming)
BLEService pumpService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE LED Service

// BLE Stepper Steps Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic stepsCharacteristic("00002a00-0000-1000-8000-00805f9b34fb", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic speedCharacteristic("1205d6e3-1a79-4b03-aa02-ce0119f79263", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic modeCharacteristic("aff5c383-b498-47ce-b27f-f1ad366b2bdb", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic fractionstepsCharacteristic("f664f283-8129-4236-932d-f47c8bf7fe09", BLERead | BLEWrite);


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
  pumpService.addCharacteristic(modeCharacteristic);
  pumpService.addCharacteristic(fractionstepsCharacteristic);
  BLE.addService(pumpService);

  // begin advertising BLE service:
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");

  // deactivate driver (LOW active)
  pinMode(enPin_pump, OUTPUT);
  pinMode(enPin_fractionArm, OUTPUT);
  digitalWrite(enPin_pump, HIGH); 
  digitalWrite(enPin_fractionArm, HIGH);
  // set dirPin to LOW
  pinMode(dirPin_pump, OUTPUT);
  pinMode(dirPin_fractionArm, OUTPUT);
  digitalWrite(dirPin_pump, LOW); //LOW or HIGH
  digitalWrite(dirPin_fractionArm, LOW); //LOW or HIGH
  // set stepPin to LOW
  pinMode(stepPin_pump, OUTPUT);
  pinMode(stepPin_fractionArm, OUTPUT);
  digitalWrite(stepPin_pump, LOW);
  digitalWrite(stepPin_fractionArm, LOW);
  //Set modi between 0=stealthChop and 1=spreadCycle of all connected stepper drivers (TMC220x)
  pinMode(spreadPin, OUTPUT);
  digitalWrite(spreadPin, LOW);
  //Set pump stepper to 1/16 microstepping
  pinMode(ms1Pin_pump, OUTPUT);
  digitalWrite(ms1Pin_pump, HIGH);
  pinMode(ms2Pin_pump, OUTPUT);
  digitalWrite(ms2Pin_pump, HIGH);
  //Set fraction stepper to 1/8 microstepping
  pinMode(ms1Pin_fractionArm, OUTPUT);
  digitalWrite(ms1Pin_fractionArm, LOW);
  pinMode(ms2Pin_fractionArm, OUTPUT);
  digitalWrite(ms2Pin_fractionArm, LOW);

  digitalWrite(enPin_pump, LOW); //activate driver
  digitalWrite(enPin_fractionArm, LOW); //activate driver

  // set the maximum speed in steps per second
  stepper_pump.setMaxSpeed(2000);
  stepper_fractionArm.setMaxSpeed(2000);

  // set Current Position
  stepper_pump.setCurrentPosition(0); 
  stepper_fractionArm.setCurrentPosition(0); 
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    String data_fractionsteps = "";
    int char_counter_fractionsteps = 0;

    String data_steps = "";
    int char_counter_steps = 0;

    String data_speed = "";
    int char_counter_speed = 0;
    
    // while the central is still connected to peripheral, collcet new characteristic values
    digitalWrite(LED_BUILTIN, HIGH); 
    while (central.connected()) {
      
      
      //collect 1 byte in total, which indcates the mode
      if (modeCharacteristic.written()) {
        mode_index = modeCharacteristic.value() - '0';
        Serial.println("Mode: " + String(mode_index)); 
      }
      
      //collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (fractionstepsCharacteristic.written()) {
        int value = fractionstepsCharacteristic.value() - '0';
        data_fractionsteps.concat(value);
        char_counter_fractionsteps++;
      
        if (char_counter_fractionsteps == 10) {
          steps_per_pump_fraction = data_fractionsteps.toFloat();
          Serial.println("Steps per pump fraction: " + String(steps_per_pump_fraction));
          data_fractionsteps = "";  
          char_counter_fractionsteps = 0;
        }
      }
      
      //collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (stepsCharacteristic.written()) {
        int value = stepsCharacteristic.value() - '0';
        data_steps.concat(value);
        char_counter_steps++;
      
        if (char_counter_steps == 10) {
          total_steps_pump = data_steps.toFloat();
          Serial.println("Steps: " + String(total_steps_pump));
          data_steps = "";  
          char_counter_steps = 0;     
        }
      }
      
      //collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (speedCharacteristic.written()) {
        int value = speedCharacteristic.value() - '0';
        data_speed.concat(value);
        char_counter_speed++;
      
        if (char_counter_speed == 10) {
          steps_per_second_pump = data_speed.toFloat();
          Serial.println("Speed: " + String(steps_per_second_pump));
          data_speed = "";  
          char_counter_speed = 0;     
        }
      }
    }
    // Drop/Waste mode
    if (mode_index == 0) {
      steps_per_second_fractionArm = 400;
      target_steps_fractionArm = 0;                                     // "Drop tube" position
      target_steps_pump = total_steps_pump;                             // no fraction
    }
    // Flow mode
    if (mode_index == 1) {
      steps_per_second_fractionArm = -400;
      target_steps_fractionArm = 0;                                     // "Drop tube" position
      target_steps_pump = total_steps_pump - steps_per_pump_fraction;   // one "Flow tube" fraction at the end  
    }
    // Elute mode
    if (mode_index == 2) {
      steps_per_second_fractionArm = 400;
      target_steps_fractionArm = fractionArm_switchSteps;               // Start at frist fraction tube
      target_steps_pump = steps_per_pump_fraction;                      // multiple fractions, vairable volume 
    }

    // reset stepper position to zero
    stepper_pump.setCurrentPosition(0);
    stepper_fractionArm.setCurrentPosition(0);
    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, LOW); 
  }

  //Move pump stepper motor as long not all steps have been perfomed 
  if (stepper_pump.currentPosition() != total_steps_pump) {
    stepper_pump.setSpeed(steps_per_second_pump);
    stepper_pump.runSpeed();
  }
  
  //Move fraction arm as long not all steps have been perfomed
  if (stepper_fractionArm.currentPosition() != target_steps_fractionArm) {
    stepper_fractionArm.setSpeed(steps_per_second_fractionArm);
    stepper_fractionArm.runSpeed();
  }

  //Update arm target position if new fraction begins
  if (stepper_pump.currentPosition() > target_steps_pump) {
    if(mode_index == 2){
    target_steps_fractionArm += fractionArm_switchSteps;      // Move to next tube
    target_steps_pump += steps_per_pump_fraction;             // Keep track of pump for next fraction
    }
    if(mode_index == 1){
    target_steps_fractionArm = -fractionArm_switchSteps;      // Move to flow fraction tube
    target_steps_pump += steps_per_pump_fraction;             // Keep track of pump for next fraction
    }
  }

  //Bring fraction arm back to start position 
  if (stepper_pump.currentPosition() == total_steps_pump) {
    delayMicroseconds(3000);                                // Wait for last droplet
    while(stepper_fractionArm.currentPosition() != 0){
      stepper_fractionArm.setSpeed(-steps_per_second_fractionArm);
      stepper_fractionArm.runSpeed();
    }
  }

}
