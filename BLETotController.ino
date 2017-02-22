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



BLEPeripheral bleController; //set the name of the arduino for BLE
BLEService controlService("9456767d-d15b-41a3-a697-4e2aa8f36ad1"); //create the BLE sevice
BLECharCharacteristic bleCharacteristic("7d99f31c-bc4f-4087-b0dc-f2ea2e7f0624", BLERead | BLEWrite);
BLECharCharacteristic speedCharacteristic("ee80b338-0bda-4b03-b54f-80cfc3d55117", BLERead | BLEWrite);

// define what ouput sets the relays to off and on
#define OFF LOW
#define ON HIGH

//set relay outputs
const int rPositive = 0;
const int rNegative = 1;
const int rThrottle = 2;
const int rBrake = 3;
const int rRPCut = 4;
const int rRNCut = 5;
const int rLPCut = 6;
const int rLNCut = 7;
const int rRP = 8;
const int rRN = 9;
const int rLP = 10;
const int rLN = 11;
 //set test button inputs
const int testButton1 = A2;
const int testButton2 = A3;
const int testButton3 = A4;
const int testButton4 = A5;
int test1State = 1;
int test2State = 1;
int test3State = 1;
int test4State = 1;
int test1PrevState = 1;
int test2PrevState = 1;
int test3PrevState = 1;
int test4PrevState = 1;
int bleState = 0;
const int debounceDelay = 50;
unsigned long debounceTime1 = 0; 
int buttonState = 0;
unsigned long forwardTimer = 0;
unsigned long reverseTimer = 0;
//unsigned long brakeTimer = 0;
const long timer = 1000;
long brakePoint = 2000;

void BLEMode (int A); //swiching BLE and Child control; 0=disabled 1=enabled
void motorRotatio (int B); //direction and speed of motor; 0=reverse, 1=lowspeed, 2=high speed

void setup() {  
  //Set all relays to outputs in the off state
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

  BLECentral controlDevice = bleController.central();

do{

  int test1State = digitalRead(testButton1);
  int test2State = digitalRead(testButton2);
  int test3State = digitalRead(testButton3);
  int test4State = digitalRead(testButton4);

  if(test1State == 0 || bleCharacteristic.value() == 0){
    test1State = 0;
  }

  if(test2State == 0 || speedCharacteristic.value() == 'R'){
    test2State = 0;
  }
  
  if(test3State == 0 || speedCharacteristic.value() == '1'){
    test3State = 0;
  }
  
  if(test4State == 0 || speedCharacteristic.value() == '2'){
    test4State = 0;
  }

  //test turning BLE Control on and off
  if (test1State != test1PrevState){
    debounceTime1 = millis();
  }


  if ((millis() - debounceTime1) > debounceDelay) {
    if (test1State != buttonState) {
      buttonState = test1State;
    
      if (buttonState == LOW) {
        bleState = !bleState;
        BLEMode(bleState);
      }
    }
  }
  test1PrevState = test1State;

  //Test reverse
  if((millis()-forwardTimer) > timer){
    if(test2State == 0 && test2PrevState == 1){
      motorRotatio(0);
      test2PrevState = 0;
    }else if(test2State == 1 && test2PrevState == 0){
      digitalWrite(rThrottle, OFF);
      reverseTimer = millis();
      test2PrevState = 1;
    }
  }

  //Test low speed
  if((millis()- reverseTimer) > timer){
    if(test3State == 0 && test3PrevState == 1){
      motorRotatio(1);
      test3PrevState = 0;
    }else if(test3State == 1 && test3PrevState == 0){
      digitalWrite(rThrottle, OFF);
      forwardTimer = millis();
      test3PrevState = 1;
    }
  }

  //Test High speed 
  if((millis()- reverseTimer) > timer){
    if(test4State == 0 && test4PrevState == 1){
      motorRotatio(2);
      test4PrevState = 0;
    }else if(test4State == 1 && test4PrevState == 0){
      digitalWrite(rThrottle, OFF);
      forwardTimer = millis();
      test4PrevState = 1;
    }
  }
  if(test2State == 1 && test3State == 1 && test4State == 1){
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

  //Turn on/off BLE control of the vehicle by Turning off the throttle, brake, and motor control relays (to prvent shorts or unwanted startup) 
  //then turning on/off the Positive, negative, and motor cutout relays
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

void motorRotatio (int B){

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
