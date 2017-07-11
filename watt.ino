// Watt? Smart Meter

#include "watt.h"



void setup() {

  //gpio configurations
  gpioConfig();

  //Wifi configuration and connection
  wificonfig_start();

  //connect to firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //eeprom setup
  eepromBegin(512);

  //initialize time
  time_init();

  //load saved data from eeprom
  loadSavedData();

  //sync saved cumulative values with firebase  
  syncCumulative();

  //initialize prevTime to now
  setPrevTime(now());

}


void loop() {

  double elapsedTime = now() - getPrevTime();

  //handling interrupt
  if(getInterruptTime() != 0)  
    handleInterrupt();

  if(elapsedTime >= 60)
    cumulate(elapsedTime);

  float realTime = readRealTime();
  Firebase.setFloat(String("Homes/")+METER_ID+"/consumption/realtime", realTime);
  setRealTimeSum(getRealTimeSum() + realTime);
  incRealTimeCount();

  //just for debugging
  Serial.println(String("Real-time Power = ") + realTime);
  Serial.println(String("Cumulative Power = ") + getCumulative());
  Serial.println();
  
  delay(500);
}

