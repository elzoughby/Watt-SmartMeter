#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <FirebaseArduino.h>    //https://github.com/firebase/firebase-arduino
#include "watt_eeprom.h"
#include "watt_time.h"
#include "watt_wificonfig.h"

#define FIREBASE_HOST "watt-project-eg.firebaseio.com"
#define FIREBASE_AUTH "BIiq1X5t2MYbzj9mQxat1BuABRNIX8VT7YGIz7Mb"
#define METER_ID "-Knn2Qr5N2DhTCjz9IVq"

#define CONTROL_BUTTON 5
#define WIFI_LED 2




void gpioConfig();

void loadSavedData();

float readRealTime();

void syncCumulative();

void cumulate(double elapsedTime);

double getCumulative();

time_t getPrevTime();

void setPrevTime(time_t newVal);

double getRealTimeSum();

void setRealTimeSum(double newVal);

double getRealTimeCount();

void incRealTimeCount();

time_t getInterruptTime();

void handleInterrupt();
