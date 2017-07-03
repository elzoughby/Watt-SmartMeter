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



unsigned float prevPowerAverage = 0;
unsigned double cumulative = 0;
unsigned double hourCumulative = 0
unsigned double dayCumulative = 0;
unsigned double monthCumulative = 0;
unsigned double yearCumulative = 0;
unsigned char currHour = 0;
unsigned char currDay = 0;
unsigned char currMonth = 0;
unsigned short currYear = 0;
unsigned float realTimeSum = 0;
unsigned short realTimeCount = 0;
time_t prevTime;


void setup() {

  //gpio configurations
  Serial.begin(9600);
  pinMode(A0, INPUT);

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

  //eeprom setup
  eepromBegin(512);

  //initialize time
  time_init();

  //load saved data from eeprom
  loadSavedData();

  //sync saved cumulative values with firebase  
  syncCumulative();

  prevTime = now();

}


void loop() {

  double elapsedTime = now() - prevTime;
  if(elapsedTime >= 60)
    calcCumulative(elapsedTime);
  float realTime = readRealTime();
  Firebase.set(String("Homes/")+METER_ID+"/consumption/realtime", realTime);
  realTimeSum += realTime;
  realTimeCount += 1;

  //just for debugging
  Serial.println(String("Real-time Power = ") + realTime);
  Serial.println(String("Cumulative Power = ") + cumulative);
  Serial.println();
  
  delay(500);
}


void loadSavedData() {

  cumulative = eepromRead(CUMULATIVE_ADDRESS);
  yearCumulative = eepromRead(YEAR_CUMULATIVE_ADDRESS);
  monthCumulative = eepromRead(MONTH_CUMULATIVE_ADDRESS);
  dayCumulative = eepromRead(DAY_CUMULATIVE_ADDRESS);
  hourCumulative = eepromRead(HOUR_CUMULATIVE_ADDRESS);
  
  currYear = eepromRead(CURR_YEAR_ADDRESS);
  currMonth = eepromRead(CURR_MONTH_ADDRESS);
  currDay = eepromRead(CURR_DAY_ADDRESS);
  currHour = eepromRead(CURR_HOUR_ADDRESS);

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
  Firebase.set(String("Homes/")+METER_ID+"/consumption/current/overall", cumulative);
  
  //sync year cumulative with firebase
  if(currYear == year())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/year", yearCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/years/"+currYear, yearCumulative);
    yearCumulative = 0;
    currYear = year();
    eepromStore(CURR_YEAR_ADDRESS, currYear);
  }

  //sync month cumulative with firebase
  if(currMonth == month())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/month", monthCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/months/"+currMonth, monthCumulative);
    monthCumulative = 0;
    currMonth = month();
    eepromStore(CURR_MONTH_ADDRESS, currMonth);
  }

  //sync day cumulative with firebase
  if(currDay == day())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/day", dayCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/days/"+currDay, dayCumulative);
    dayCumulative = 0;
    currDay = day();
    eepromStore(CURR_DAY_ADDRESS, currDay);
  }

  //sync hour cumulative with firebase
  if(currHour == hour())
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/hour", hourCumulative);
  else {
    Firebase.set(String("Homes/")+METER_ID+"/consumption/past/hours/"+currHour, hourCumulative);
    hourCumulative = 0;
    currHour = hour();
    eepromStore(CURR_HOUR_ADDRESS, currHour);
  }

}


void calcCumulative(double elapsedTime) {

    prevTime = now();
    double realTimeAverage = (double) realTimeSum / realTimeCount;
    double tempCumulative = (realTimeAverage/1000)*(elapsedTime/3600);
    
    if(currHour != hour()) {
      currHour = hour();
      hourCumulative = 0;
      eepromStore(CURR_HOUR_ADDRESS, currHour);

      if(currDay != day()) {
        currDay = day();
        dayCumulative = 0;
        eepromStore(CURR_DAY_ADDRESS, currDay);

        if(currMonth != month()) {
          currMonth = month();
          monthCumulative = 0;
          eepromStore(CURR_MONTH_ADDRESS, currMonth);
          
          if(currYear != year()) {
            currYear = year();
            yearCumulative = 0;
            eepromStore(CURR_YEAR_ADDRESS, currYear);
          }
        }
      }
    }

    hourCumulative += tempCumulative;
    eepromStore(HOUR_CUMULATIVE_ADDRESS, hourCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/hour", hourCumulative);
    
    dayCumulative += tempCumulative;
    eepromStore(DAY_CUMULATIVE_ADDRESS, dayCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/day", dayCumulative);
    
    monthCumulative += tempCumulative;
    eepromStore(MONTH_CUMULATIVE_ADDRESS, monthCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/month", monthCumulative);

    yearCumulative += tempCumulative;
    eepromStore(YEAR_CUMULATIVE_ADDRESS, yearCumulative);
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/year", yearCumulative);

    cumulative += tempCumulative;
    Firebase.set(String("Homes/")+METER_ID+"/consumption/current/overall", cumulative);
    eepromStore(CUMULATIVE_ADDRESS, cumulative);
    
    realTimeSum = 0;
    realTimeCount = 0;
    
}
