/*  BLE controller for childs ride-on electric vehicle
 *   relay outputs match BLEcontroller_v1.fzz schematic 
 *   (save for; positive source needs to be added to code)
 *  
 *  notes
 *  curently; 
 *  test button 1 turns on/off ble control
 *  test button 2 put vehicle in reverse and turns on throttle while pressed
 *  test button 3 put vehicle in low speed and turns on throttle while pressed
 *  test button 4 put vehicle in high speed and turns on throttle while pressed
 * 
 *  changes to be made;
 *  test button 1 to stay as is
 *  test button 2 to toggle between ble speed controll only (throttle controlled by child)
 *  test button 3 to toggle between reverse, low, and high speeds
 *  test button 4 = throttle when in ble full control mode
 *  
 *  2/28/17 changed variable names
 *  
 */
#include <BLEAttribute.h>
#include <BLECentral.h>
#include <BLECharacteristic.h>
#include <BLECommon.h>
#include <BLEDescriptor.h>
#include <BLEPeripheral.h>
#include <BLEService.h>
#include <BLETypedCharacteristic.h>
#include <BLETypedCharacteristics.h>
#include <BLEUuid.h>
#include <CurieBLE.h>


//set the name of the arduino for BLE
BLEPeripheral bleController;
//create the BLE sevice with characteristics
BLEService controlService("9456767d-d15b-41a3-a697-4e2aa8f36ad1"); 
BLECharCharacteristic bleCharacteristic("7d99f31c-bc4f-4087-b0dc-f2ea2e7f0624", BLERead | BLEWrite);
BLECharCharacteristic speedCharacteristic("ee80b338-0bda-4b03-b54f-80cfc3d55117", BLERead | BLEWrite);

/* define what ouput LOW/HIGH sets the relays to OFF and ON
 *  using transistors to control the SainSmart Relay Module;
 *  Arduino outputs need to be set to low to turn the relays off
 *  and high to turn them on
 */
#define OFF LOW
#define ON HIGH

//set relays
const int rPositive = 6; // Positive lead from battery
const int rNegative = 1; // Negative lead to battery
const int rThrottle = 5; // Throttle relay
const int rBrake = 4;    // Brake Relay

// relays to cut motor leads over to BLE controller
const int rRPCut = 3;    
const int rRNCut = A1;
const int rLPCut = 0;
const int rLNCut = 12;

// relays to control power to motor leads (rRP = relay Right Positive)
const int rRP = 2;
const int rRN = A0;
const int rLP = 1;
const int rLN = 13;

// set test button inputs
const int testButton1 = A2;
const int testButton2 = A3;
const int testButton3 = A4;
const int testButton4 = A5;

int blePower = 1;
int throttleSelect = 1;
int gearSelect = 1;
int bleThrottle = 1;
int blePowerPrevState = 1;
int throttleSelectPrevState = 1;
int gearSelectPrevState = 1;
int bleThrottlePrevState = 1;
int bleState = 0;

const int debounceDelay = 50;
unsigned long debounceTime1 = 0; 
unsigned long debounceTime2 = 0;
unsigned long debounceTime3 = 0;
int buttonState = 0;
unsigned long forwardTimer = 0;
unsigned long reverseTimer = 0;
const long timer = 1000;
long brakePoint = 2000;

void BLEMode (int A); //swiching BLE and Child control; 0=disabled 1=enabled
void motorRotation (int B); //direction and speed of motor; 0=reverse, 1=lowspeed, 2=high speed

void setup() {  
  
  // Set all relays to outputs in the off state
  pinMode(rPositive,OUTPUT);
  digitalWrite(rPositive,OFF);
  pinMode(rNegative,OUTPUT);
  digitalWrite(rNegative,OFF);
  pinMode(rThrottle,OUTPUT);
  digitalWrite(rThrottle,OFF);
  pinMode(rBrake,OUTPUT);
  digitalWrite(rBrake,OFF);
  pinMode(rRPCut,OUTPUT);
  digitalWrite(rRPCut,OFF);
  pinMode(rRNCut,OUTPUT);
  digitalWrite(rRNCut,OFF);
  pinMode(rLPCut,OUTPUT);
  digitalWrite(rLPCut,OFF);
  pinMode(rLNCut,OUTPUT);
  digitalWrite(rLNCut,OFF);
  pinMode(rRP,OUTPUT);
  digitalWrite(rRP,OFF);
  pinMode(rRN,OUTPUT);
  digitalWrite(rRN,OFF);
  pinMode(rLP,OUTPUT);
  digitalWrite(rLP,OFF);
  pinMode(rLN,OUTPUT);
  digitalWrite(rLN,OFF);

  // set test button inputs
  pinMode(testButton1, INPUT_PULLUP);
  pinMode(testButton2, INPUT_PULLUP);
  pinMode(testButton3, INPUT_PULLUP);
  pinMode(testButton4, INPUT_PULLUP);


  bleController.setLocalName("Control");
  bleController.setAdvertisedServiceUuid(controlService.uuid());

  bleController.addAttribute(controlService);
  bleController.addAttribute(bleCharacteristic);
  bleController.addAttribute(speedCharacteristic);

  bleCharacteristic.setValue(0);
  speedCharacteristic.setValue(0);

  bleController.begin();
}

void loop() {
  // attempt ble connection
  BLECentral controlDevice = bleController.central();

  // do-while loop alows code to run whether ble connection is made or not
  do{
    
    blePower = digitalRead(testButton1);
    throttleSelect = digitalRead(testButton2);
    gearSelect = digitalRead(testButton3);
    bleThrottle = digitalRead(testButton4);

    if(blePower == 0 || bleCharacteristic.value() == 0){
      blePower = 0;
    }

    if(throttleSelect == 0 || speedCharacteristic.value() == 'R'){
      throttleSelect = 0;
    }
  
    if(gearSelect == 0 || speedCharacteristic.value() == '1'){
      gearSelect = 0;
    }
  
    if(bleThrottle == 0 || speedCharacteristic.value() == '2'){
      bleThrottle = 0;
    }

    // **************************turn BLE Control on and off********************************************
    if (blePower != blePowerPrevState){
      debounceTime1 = millis();
    }

    if ((millis() - debounceTime1) > debounceDelay) {
      if (blePower != buttonState) {
        buttonState = blePower;
    
        if (buttonState == LOW) {
          bleState = !bleState;
          BLEMode(bleState);
        }
      }
    }
    blePowerPrevState = blePower;
    // ***********************************************************************************************

    //Test reverse
    if((millis()-forwardTimer) > timer){
      if(throttleSelect == 0 && throttleSelectPrevState == 1){
        motorRotation(0);
        throttleSelectPrevState = 0;
      }else if(throttleSelect == 1 && throttleSelectPrevState == 0){
        digitalWrite(rThrottle, OFF);
        reverseTimer = millis();
        throttleSelectPrevState = 1;
      }
    }

    //Test low speed
    if((millis()- reverseTimer) > timer){
      if(gearSelect == 0 && gearSelectPrevState == 1){
        motorRotation(1);
        gearSelectPrevState = 0;
      }else if(gearSelect == 1 && gearSelectPrevState == 0){
        digitalWrite(rThrottle, OFF);
        forwardTimer = millis();
        gearSelectPrevState = 1;
      }
    }

    //Test High speed 
    if((millis()- reverseTimer) > timer){
      if(bleThrottle == 0 && bleThrottlePrevState == 1){
        motorRotation(2);
        bleThrottlePrevState = 0;
      }else if(bleThrottle == 1 && bleThrottlePrevState == 0){
        digitalWrite(rThrottle, OFF);
        forwardTimer = millis();
        bleThrottlePrevState = 1;
      }
    }
    if(throttleSelect == 1 && gearSelect == 1 && bleThrottle == 1){
      if((millis() - reverseTimer) > brakePoint && (millis() - forwardTimer) > brakePoint){
        digitalWrite(rBrake, OFF);
      }
    }
  }while (controlDevice.connected());
}

void BLEMode(int A) {

  if (A < 0 || A > 1){
    return; 
  }

  /* Turn on/off BLE control of the vehicle by;
  *  Turning control relays to normal state (to prvent shorts or unwanted startup) 
  *  then turning on/off the Positive, negative, and motor cutout relays
  */
  digitalWrite(rThrottle, OFF);
  digitalWrite(rBrake, OFF);
  digitalWrite(rRP, OFF);
  digitalWrite(rRN, OFF);
  digitalWrite(rLP, OFF);
  digitalWrite(rLN, OFF);

  if (A == 1){
    digitalWrite(rPositive, ON);
    digitalWrite(rNegative, ON);
    digitalWrite(rRPCut, ON);
    digitalWrite(rRNCut, ON);
    digitalWrite(rLPCut, ON);
    digitalWrite(rLNCut, ON);
  }else if (A == 0){
    digitalWrite(rPositive, OFF);
    digitalWrite(rNegative, OFF);
    digitalWrite(rRPCut, OFF);
    digitalWrite(rRNCut, OFF);
    digitalWrite(rLPCut, OFF);
    digitalWrite(rLNCut, OFF);
  }
  return;
}

void motorRotation (int B){

  if (B < 0 || B > 2){
    return;
  }

  digitalWrite(rThrottle, OFF);

  if (B == 0){
    digitalWrite(rRP, ON);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, ON);
  }else if (B == 1){
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, OFF);
  }else if (B == 2){
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, ON);
    digitalWrite(rLP, ON);
    digitalWrite(rLN, OFF);
  }

  digitalWrite(rThrottle, ON);
  digitalWrite(rBrake, ON);

  return;
}
