/*  BLE controller for childs ride-on electric vehicle
 *   relay outputs match BLEcontroller.fzz schematic 
 *  
 *  
 *  2/28/17 changed variable names
 *  3/3/17 changed both functions
 *  3/4/17 added bleThrottle 
 *  3/17/17 added dont allow shifting if throttle is on
 *    and delays for shifting between forward and reverse gears
 *  3/20/17 changed positve source relay usage
 *  3/23/17 updated comments
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
BLECharCharacteristic gearCharacteristic("735e4d05-fb92-4e72-adb5-0202c5e4310d", BLERead | BLEWrite);

/* define what ouput LOW/HIGH sets the relays to OFF and ON
 *  using transistors to control the SainSmart Relay Module;
 *  Arduino outputs need to be set to low to turn the relays off
 *  and high to turn them on
 */
#define OFF LOW
#define ON HIGH

//set relays
const int rSource = 7;    // source of positive power battery or throttle
const int rPositive = 6;  // Positive cut-out
const int rNegative = 11; // Negative lead to battery
const int rThrottle = 5;  // Throttle relay
const int rBrake = 4;     // Brake Relay

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
//const int testButton1 = A2;
unsigned long debounceTime1 = 0;
//const int testButton2 = A3;
unsigned long debounceTime2 = 0;
//const int testButton3 = A4;
unsigned long debounceTime3 = 0;
//const int testButton4 = A5;
unsigned long debounceTime4 = 0;

int blePower = 1;
int blePowerPrevState = 1;
int bleState = 0;
int button1State = 0;

int throttleSelect = 1;
int throttleSelectPrevState = 1;
int throttleState = 1;
int button2State = 0;

int gearSelect = 1;
int gearSelectPrevState = 1;
int gearSelectState = 0;
int button3State = 0;
int currentGear = 1;              //set set gear to LOW gear on startup

int bleThrottle = 1;
int bleThrottlePrevState = 1;
int button4State = 0;

int brakeState = 0;               //to let other functions/actions know when the brakes are engaged
const int debounceDelay = 50;
unsigned long throttleTimer = 0;  // used to set time from throttle-off to brake-engage
const long timer = 500;


void BLEMode (int A, int B);      //swiching BLE and Child control; 0=BLE disabled 1=BLE enabled
void motorRotation (int C);       //direction and speed of motor; 0=reverse, 1=lowspeed, 2=high speed

void setup(){  
  
  // Set all relays to outputs in the off state
  pinMode(rSource, OUTPUT);
  digitalWrite(rSource, OFF);
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
  //pinMode(testButton1, INPUT_PULLUP);
  //pinMode(testButton2, INPUT_PULLUP);
  //pinMode(testButton3, INPUT_PULLUP);
  //pinMode(testButton4, INPUT_PULLUP);


  bleController.setLocalName("Control");
  bleController.setAdvertisedServiceUuid(controlService.uuid());

  bleController.addAttribute(controlService);
  bleController.addAttribute(bleCharacteristic);
  bleController.addAttribute(speedCharacteristic);
  bleController.addAttribute(gearCharacteristic);

  bleCharacteristic.setValue(0);
  speedCharacteristic.setValue(0);

  bleController.begin();
}

void loop(){
  // attempt ble connection
  BLECentral controlDevice = bleController.central();

  // do-while loop checks for ble connection
  do{

    // **************************turn BLE Control on and off******************************************
    
    if(/*digitalRead(testButton1) == 0 ||*/ bleCharacteristic.value() == 1){
      blePower = 0;
    }else{
      blePower = 1;
    }
    
    if(blePower != blePowerPrevState){
      debounceTime1 = millis();
    }

    if((millis() - debounceTime1) > debounceDelay){
      if(blePower != button1State){
        button1State = blePower;
    
        if(button1State == LOW){
          bleState = !bleState;
          BLEMode(bleState, throttleState);
        }
      }
    }
    blePowerPrevState = blePower;
    
    // ***********************************************************************************************

    //***************Throttle select: swaps power between in-car controls and BLE controls ***********
    
    if(/*digitalRead(testButton2) == 0 ||*/ speedCharacteristic.value() == 1){
      throttleSelect = 0;
    }else{
      throttleSelect = 1;
    }
    
    if(throttleSelect != throttleSelectPrevState){
      debounceTime2 = millis();
    }

    if((millis() - debounceTime2) > debounceDelay){
      if(throttleSelect != button2State){
        button2State = throttleSelect;
    
        if(button2State == LOW && brakeState == 0){  //checking brake state makes sure car is stoped before switching control over
          throttleState = !throttleState;
          BLEMode(bleState, throttleState);
        }
      }
    }
    throttleSelectPrevState = throttleSelect;
    
    //************************************************************************************************
    
    // ***************************Gear select: changes gears(duh)*************************************

    //int analogReadOf3 = analogRead(testButton3);          //test button 3 is not functional 

    if((/*analogReadOf3 <= 256 ||*/ gearCharacteristic.value() == 1) && bleThrottle == 1){
      gearSelect = 0;
    }else if((/*(analogReadOf3 > 256 && analogReadOf3 < 768) ||*/ gearCharacteristic.value() == 'R') && bleThrottle == 1){
      gearSelect = 'R';
    }else{
      gearSelect = 1;
    }

    if(gearSelect != gearSelectPrevState){
      debounceTime3 = millis();
    }

    if((millis() - debounceTime3) > debounceDelay){
      if(gearSelect != button3State){
        button3State = gearSelect;

        if(button3State == 'R' && currentGear != 0 && brakeState == 0){
          currentGear = 0;
          motorRotation(currentGear);
        }else if(button3State == 0 && currentGear != 1 && bleThrottle == 1){
          currentGear = 1;
          motorRotation(currentGear);
        }else if(button3State == 0 && currentGear == 1 && bleThrottle == 1){
          currentGear = 2;
          motorRotation(currentGear);
        }
      }
    }
    gearSelectPrevState = gearSelect;
    
    // ***********************************************************************************************
    
    // ***************************Throttle************************************************************
  
    if((/*digitalRead(testButton4) == 0 || */speedCharacteristic.value() == 'G') && throttleState == 1){
      bleThrottle = 0;
    }else{
      bleThrottle = 1;
    }

    if(bleThrottle != bleThrottlePrevState){
      debounceTime4 = millis();
    }

    if((millis() - debounceTime4) > debounceDelay){
      if(bleThrottle != button4State){
        button4State = bleThrottle;
        if(button4State == LOW){
          digitalWrite(rBrake, ON);
          brakeState = 1;
          digitalWrite(rThrottle, ON);
        }

        if(bleThrottle == HIGH){
          digitalWrite(rThrottle, OFF);
          throttleTimer = millis();
          brakeState = 2;
        }
      }
    }
    bleThrottlePrevState = bleThrottle;

    // ***********************************************************************************************

    if(throttleSelect == 1 && gearSelect == 1 && bleThrottle == 1 && throttleState == 1 && brakeState == 2){
      if((millis() - throttleTimer) > timer){
        digitalWrite(rBrake, OFF);
        brakeState = 0;
      }
    }
  }while(controlDevice.connected());

}//end main loop


void BLEMode(int A, int B){

  /* Turn on/off BLE control of the vehicle by;
  *  Turning control relays to normal state (to prvent shorts or unwanted startup) 
  *  then turning on/off the Positive, negative, and motor cutout relays
  */
  //digitalWrite(rSource, OFF);
  digitalWrite(rThrottle, OFF);
  digitalWrite(rBrake, OFF);
  //digitalWrite(rRP, OFF);
  //digitalWrite(rRN, OFF);
  //digitalWrite(rLP, OFF);
  //digitalWrite(rLN, OFF);

  // if turning BLE off
  if(A == 0){
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, OFF);
    digitalWrite(rSource, OFF);
    digitalWrite(rPositive, OFF);
    digitalWrite(rNegative, OFF);
    digitalWrite(rRPCut, OFF);
    digitalWrite(rRNCut, OFF);
    digitalWrite(rLPCut, OFF);
    digitalWrite(rLNCut, OFF);
    
  // if turning BLE on (A) without BLE throttle control (B) 
  }else if(A == 1 && B == 0){
    digitalWrite(rSource, OFF);
    digitalWrite(rPositive, ON);
    digitalWrite(rNegative, ON);
    digitalWrite(rRPCut, ON);
    digitalWrite(rRNCut, ON);
    digitalWrite(rLPCut, ON);
    digitalWrite(rLNCut, ON);
    digitalWrite(rThrottle, ON);
    digitalWrite(rBrake, ON);
    
  // if turning BLE on (A) with BLE throttle control (B) 
  }else if(A == 1 && B == 1){
    digitalWrite(rSource, ON);
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, OFF);
    digitalWrite(rPositive, ON);
    digitalWrite(rNegative, ON);
    digitalWrite(rRPCut, ON);
    digitalWrite(rRNCut, ON);
    digitalWrite(rLPCut, ON);
    digitalWrite(rLNCut, ON);
  }
  return;
}

void motorRotation (int C){

  if(throttleState == 0){
    BLEMode(1, 1);
    delay(timer);
  }
  
  // Reverse
  if(C == 0){
    digitalWrite(rRP, ON);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, ON);
  
  // Low speed
  }else if(C == 1){
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, OFF);
    digitalWrite(rLP, OFF);
    digitalWrite(rLN, OFF);

  // High speed
  }else if(C == 2){
    digitalWrite(rRP, OFF);
    digitalWrite(rRN, ON);
    digitalWrite(rLP, ON);
    digitalWrite(rLN, OFF);
  }
  
  if(throttleState == 0){
    BLEMode(1, 0);
  }

  return;
}
