#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"

//Set these OTAA parameters to match your app/node in TTN
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x05, 0x3A, 0x68 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x52, 0xA9, 0xB2, 0xDF, 0xD5, 0x34, 0xF2, 0x06, 0x7E, 0x3D, 0xD8, 0xD9, 0x14, 0x49, 0x35, 0xC6 };

// Constants
const int sensorList[3] = {GPIO1, GPIO2, GPIO3};
const int delayTime = 10; // Time of acquisition of data for each sensor, in seconds
const int sleepTime = 180; // Time into sleepmode, in seconds
const int sensorNumber = sizeof(sensorList)/sizeof(int);
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

// Variables
int sensorCounter = 0;   // counter for the number of button presses
int sensorState = 0;         // current state of the button
int lastSensorState = 0;     // previous state of the button
unsigned long myTime = 0;
uint8_t sensorValues[] = {0, 0, 0};

static uint8_t counter=0;
uint8_t lora_data[3];
uint8_t downlink ;

//Some utilities for going into low power mode
TimerEvent_t sleepTimer;
//Records whether our sleep/low power timer expired
bool sleepTimerExpired;

static void wakeUp()
{
  sleepTimerExpired=true;
}

static void lowPowerSleep(uint32_t sleeptime)
{
  sleepTimerExpired=false;
  TimerInit( &sleepTimer, &wakeUp );
  TimerSetValue( &sleepTimer, sleeptime );
  TimerStart( &sleepTimer );
  //Low power handler also gets interrupted by other timers
  //So wait until our timer had expired
  while (!sleepTimerExpired) lowPowerHandler();
  TimerStop( &sleepTimer );
}

///////////////////////////////////////////////////

void setup() {
  pinMode(GPIO4, OUTPUT);
  digitalWrite(GPIO4, LOW);
  for (int i = 0; i < sensorNumber ; i++) {
    pinMode(sensorList[i], INPUT);
  }
  Serial.begin(115200);
  delay(1000);
  Serial.println("--------- BubbleSensor --------");
  Serial.print("number of sensors : ");
  Serial.println(sizeof(sensorList)/sizeof(int));

  LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION);
  
  //Enable ADR
  LoRaWAN.setAdaptiveDR(true);

  while (1) {
    Serial.print("Joining... ");
    LoRaWAN.joinOTAA(appEui, appKey, devEui);
    if (!LoRaWAN.isJoined()) {
      //In this example we just loop until we're joined, but you could
      //also go and start doing other things and try again later
      Serial.println("JOIN FAILED! Sleeping for 30 seconds");
      lowPowerSleep(30000);
    } else {
      Serial.println("JOINED");
      break;
    }
  }
  
}

void loop() {
  counter++; 
  delay(10);
  Serial.println("----- Hello World ! -----");
  //For each sensor :
  for (int i = 0; i < sensorNumber ; i++) {

    Serial.print("Sensor n° : ");
    Serial.println(i+1);
    
    // During 10 seconds :
    Serial.println("Start acquisition");
    myTime = millis();
    while ( myTime + delayTime*1000 > millis() ) {
      int sensor = sensorList[0];
      sensorState = digitalRead(sensorList[i]);
      // compare the sensorState to its previous state
      if (sensorState != lastSensorState) {
        // if the state has changed, increment the counter
        if (sensorState == HIGH) {
          // if the current state is HIGH then the button went from off to on:
          sensorCounter++;
          //Serial.println("on");
          //Serial.print("number of detections : ");
          //Serial.println(sensorCounter);
        } else {
          // if the current state is LOW then the button went from on to off:
          //Serial.println("off");
        }
        // Delay a little bit to avoid bouncing
        delay(50);
      }
      // save the current state as the last state, for next time through the loop
      lastSensorState = sensorState;
    }
    sensorValues[i] = sensorCounter;
    Serial.print("End of acquisition. Total of detections : ");
    Serial.println(sensorCounter);

    //Reset variables for next sensor
    sensorCounter = 0;   // counter for the number of button presses
    sensorState = 0;         // current state of the button
    lastSensorState = 0;     // previous state of the button
  }
  Serial.println("sensorValues : ");
  for (int i = 0; i < sensorNumber ; i++) {
    lora_data[i]=sensorValues[i];
    Serial.print(sensorValues[i]);
    Serial.print(" . ");
  }
  Serial.println("End of acquisition.");
  //Now send the data. The parameters are "data size, data pointer, port, request ack"
  Serial.printf("\nSending packet with counter=%d\n", counter);
  Serial.printf("\nValue to send : %d\n", lora_data[1]);

  //Here we send confirmed packed (ACK requested) only for the first two (remember there is a fair use policy)
  bool requestack=counter<2?true:false;
  if (LoRaWAN.send(sizeof(lora_data), lora_data, 1, requestack)) {
    Serial.println("Send OK");
  } else {
    Serial.println("Send FAILED");
  }

  lowPowerSleep(sleepTime*1000);  
  delay(10);}
