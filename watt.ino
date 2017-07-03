// Watt?


#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <FirebaseArduino.h>    //https://github.com/firebase/firebase-arduino
#include "watt_eeprom.h"
#include "watt_time.h"

#define FIREBASE_HOST "https://watt-project-eg.firebaseio.com/"
#define FIREBASE_AUTH "BIiq1X5t2MYbzj9mQxat1BuABRNIX8VT7YGIz7Mb"
#define METER_ID "-Knn2Qr5N2DhTCjz9IVq"

#define WIFI_SSID "Zox"
#define WIFI_PASSWORD "The1stZox"



float prevPowerAverage = 0;
float realTime = 0;
double cumulative = 0;
double hourCumulative = 0
double dayCumulative = 0;
double monthCumulative = 0;
double yearCumulative = 0;
unsigned char currHour = 0;
unsigned char currDay = 0;
unsigned char currMonth = 0;
unsigned short currYear = 0;
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

  //sync saved cumulative values with firebase
  syncCumulative();

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
  Firebase.set(String("Homes/")+METER_ID+"/consumption/realtime", realTime);
  realTimeSum += realTime;
  realTimeCount += 1;

  //just for debugging
  Serial.println(String("Real-time Power = ") + realTime);
  Serial.println(String("Cumulative Power = ") + cumulative);
  Serial.println();
  
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


void syncCumulative() {

  //sync overall cumulative with firebase
  cumulative = eepromRead(CUMULATIVE_ADDRESS);
  Firebase.set(String("Homes/")+METER_ID+"/consumption/current/overall", cumulative);
  
  //sync year cumulative with firebase
  yearCumulative = eepromRead(YEAR_CUMULATIVE_ADDRESS);
  currYear = eepromRead(CURR_YEAR_ADDRESS);
  if(currYear == year())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/year", yearCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/years/"+currYear, yearCumulative);
    yearCumulative = 0;
    currYear = year();
  }

  //sync month cumulative with firebase
  monthCumulative = eepromRead(MONTH_CUMULATIVE_ADDRESS);
  currMonth = eepromRead(CURR_MONTH_ADDRESS);
  if(currMonth == month())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/month", monthCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/months/"+currMonth, monthCumulative);
    monthCumulative = 0;
    currMonth = month();
  }

  //sync day cumulative with firebase
  dayCumulative = eepromRead(DAY_CUMULATIVE_ADDRESS);
  currDay = eepromRead(CURR_DAY_ADDRESS);
  if(currDay == day())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/day", dayCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/days/"+currDay, dayCumulative);
    dayCumulative = 0;
    currDay = day();
  }

  //sync hour cumulative with firebase
  hourCumulative = eepromRead(HOUR_CUMULATIVE_ADDRESS);
  currHour = eepromRead(CURR_HOUR_ADDRESS);
  if(currHour == hour())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/hour", hourCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/hours/"+currHour, hourCumulative);
    hourCumulative = 0;
    currHour = hour();
  }

}


void calcCumulative(double elapsedTime) {

    prevTime = now();
    double realTimeAverage = (double) realTimeSum / realTimeCount;
    cumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/overall", cumulative);
    eepromStore(CUMULATIVE_ADDRESS, cumulative);
    
    //this peice of code lacks the hour cumulative
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
    eepromStore(YEAR_CUMULATIVE_ADDRESS, yearCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/year", yearCumulative);
    
    monthCumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    eepromStore(MONTH_CUMULATIVE_ADDRESS, monthCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/month", monthCumulative);
    
    dayCumulative += (realTimeAverage/1000)*(elapsedTime/3600);
    eepromStore(DAY_CUMULATIVE_ADDRESS, dayCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/day", dayCumulative);
    
    realTimeSum = 0;
    realTimeCount = 0;
    
}
