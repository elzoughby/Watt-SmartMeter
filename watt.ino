// Watt?


#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <FirebaseArduino.h>    //https://github.com/firebase/firebase-arduino
#include "watt_eeprom.h"
#include "watt_time.h"

#define FIREBASE_HOST "https://watt-project-eg.firebaseio.com/"
#define FIREBASE_AUTH "BIiq1X5t2MYbzj9mQxat1BuABRNIX8VT7YGIz7Mb"

#define WIFI_SSID "Zox"
#define WIFI_PASSWORD "The1stZox"



float prevPowerAverage = 0;
float realTime = 0;
double cumulative = 0;
double dayCumulative = 0;
double monthCumulative = 0;
double yearCumulative = 0;
short currDay = 0;
short currMonth = 0;
unsigned int currYear = 0;
unsigned int realTimeSum = 0;
unsigned int realTimeCount = 0;
time_t prevTime;


void setup() {

  //configurations
  Serial.begin(9600);
  pinMode(A0, INPUT);

  //eeprom setup
  eepromBegin(512);

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  //just for debugging
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  //connect to firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  initiateCumulative();

  //initialize time
  time_init();
  prevTime = now();
  Serial.println(prevTime);

}


void loop() {

  double elapsedTime = now() - prevTime;
  if(elapsedTime >= 60)
    calcCumulative(elapsedTime);
  realTime = readRealTime();
  Firebase.set("Devices/-KfWxUaA7tnwZr4ZIbrq/consumption", realTime);
  realTimeSum += realTime;
  realTimeCount += 1;

  //just for debugging
  Serial.print("Real-time Power = ");
  Serial.println(realTime);
  Serial.print("Cumulative Power = ");
  Serial.println(cumulative);
  Serial.println(" ");
  
  delay(500);
}


float readRealTime() {

  float powerSum= 0;
  float powerAverage= 0;

  for(int j=0; j < 10; j++) {

    unsigned int sensorValue = 0;
    unsigned int sensorArray[50];
    unsigned int sensorMax =0;
    unsigned int sensorSum =0;
    unsigned int difference=0;
    unsigned int sensorAverage =0;
    double voltage=0;
    double current=0;
    double power=0;

    for(int i=0; i<50; i++) {
      sensorValue = analogRead(A0);
      sensorArray[i] = sensorValue;
      sensorSum += sensorValue;
      if (sensorArray[i]>sensorMax) sensorMax = sensorArray[i];
      delay(5);
    }
    sensorAverage = sensorSum/50;
    difference = sensorMax-sensorAverage;

    // just for debugging
    Serial.print("Average Analoge read = ");
    Serial.println(sensorAverage);
    Serial.print("Max Analoge read = ");
    Serial.println(sensorMax);
    Serial.print("Difference = ");
    Serial.println(difference);

    voltage = (double)difference * 3.3 / 1023;
    current = voltage / 0.1;
    current = current * 0.707;
    power = current * 220;

    // just for debugging
    Serial.print("Voltage = ");
    Serial.println(voltage);
    Serial.print("Current = ");
    Serial.println(current);
    Serial.print("Consumed Power = ");
    Serial.println(power);

    powerSum += power;
  }

  powerAverage = powerSum/10;

  // just for debugging
  Serial.print("Average Consumed Power = ");
  Serial.println(powerAverage);

  // return onChange
  if( powerAverage > (prevPowerAverage + 5) || powerAverage < (prevPowerAverage - 5)) {
      prevPowerAverage = powerAverage;
      return powerAverage;
  }
  else
    return prevPowerAverage;

}


void initiateCumulative() {

  cumulative = eepromRead(CUMULATIVE_ADDRESS);
  Firebase.set("irSensor/cumulative", cumulative);
  

  yearCumulative = eepromRead(YEAR_CUMULATIVE_ADDRESS);
  currYear = eepromRead(CURR_YEAR_ADDRESS);
  Firebase.set("irSensor/history/current/year" + (String)currYear, yearCumulative);
  if(currYear != year()) 
    yearCumulative = 0;
  currYear = year();

  monthCumulative = eepromRead(MONTH_CUMULATIVE_ADDRESS);
  currMonth = eepromRead(CURR_MONTH_ADDRESS);
  Firebase.set("irSensor/history/current/month" + (String)currMonth, monthCumulative);
  if(currMonth != month()) 
    monthCumulative = 0;
  currMonth = month();

  dayCumulative = eepromRead(DAY_CUMULATIVE_ADDRESS);
  currDay = eepromRead(CURR_DAY_ADDRESS);
  Firebase.set("irSensor/history/current/day" + (String)currDay, dayCumulative);
  if(currDay != day()) 
    monthCumulative = 0;
  currMonth = day();
}


void calcCumulative(double elapsedTime) {

    prevTime = now();
    double realTimeAverage = (double) realTimeSum / realTimeCount;
    cumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    Firebase.set("irSensor/cumulative", cumulative);
    eepromStore(CUMULATIVE_ADDRESS, cumulative);

    if(currYear != year()) {

        currYear = year();
        yearCumulative = 0;
        currMonth = month();
        monthCumulative = 0;
        currDay = day();
        dayCumulative = 0;
    } else if(currMonth != month()) {
        currMonth = month();
        monthCumulative = 0;
        currDay = day();
        dayCumulative = 0;
    } else if(currDay != day()) {
        currDay = day();
        dayCumulative = 0;
    }
    yearCumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    Firebase.set("irSensor/history/current/year" + (String)currYear, yearCumulative);
    eepromStore(YEAR_CUMULATIVE_ADDRESS, yearCumulative);
    monthCumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    Firebase.set("irSensor/history/current/month" + (String)currMonth, monthCumulative);
    eepromStore(MONTH_CUMULATIVE_ADDRESS, monthCumulative);
    dayCumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    Firebase.set("irSensor/history/current/day" + (String)currDay, dayCumulative);
    eepromStore(DAY_CUMULATIVE_ADDRESS, dayCumulative);
    realTimeSum = 0;
    realTimeCount = 0;
    
}
