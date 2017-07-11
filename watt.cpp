#include "watt.h"


float prevPowerAverage = 0;
volatile double cumulative = 0;
double hourCumulative = 0;
double dayCumulative = 0;
double monthCumulative = 0;
double yearCumulative = 0;
unsigned char currHour = 0;
unsigned char currDay = 0;
unsigned char currMonth = 0;
unsigned short currYear = 0;
volatile double realTimeSum = 0;
volatile double realTimeCount = 0;
volatile time_t prevTime = 0;
volatile time_t interruptTime = 0;


//no need to be shared
void interrupt(); 


void gpioConfig() {

  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(CONTROL_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(CONTROL_BUTTON), interrupt, FALLING);

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

    voltage = (double)difference * 3.3 / 1023;
    current = voltage / 0.1;
    current = current * 0.707;
    power = current * 220;

    powerSum += power;
  }

  powerAverage = powerSum/10;

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

void cumulate(double elapsedTime) {

    prevTime = now();
    double realTimeAverage = realTimeSum / realTimeCount;
    double tempCumulative = (realTimeAverage/1)*(elapsedTime/3600);
    //just for debugging
    Serial.println("============================");
    Serial.println(String("realTimeAverage = ") + realTimeAverage);
    Serial.println(String("tempCumulative = ") + tempCumulative);
    Serial.println("============================\n");
    
    if(currHour != hour()) {
      Firebase.set(String("Homes/")+METER_ID+"/consumption/past/hours/"+currHour, hourCumulative);
      hourCumulative = 0;
      currHour = hour();
      eepromStore(CURR_HOUR_ADDRESS, currHour);

      if(currDay != day()) {
        Firebase.set(String("Homes/")+METER_ID+"/consumption/past/days/"+currDay, dayCumulative);
        dayCumulative = 0;
        currDay = day();
        eepromStore(CURR_DAY_ADDRESS, currDay);

        if(currMonth != month()) {
          Firebase.set(String("Homes/")+METER_ID+"/consumption/past/months/"+currMonth, monthCumulative);
          monthCumulative = 0;
          currMonth = month();
          eepromStore(CURR_MONTH_ADDRESS, currMonth);
          
          if(currYear != year()) {
            Firebase.set(String("Homes/")+METER_ID+"/consumption/past/years/"+currYear, yearCumulative);
            yearCumulative = 0;
            currYear = year();
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

void interrupt() {

  //set the interrupt flag to handle in the main loop
  interruptTime = now();

}

double getCumulative() {
    return cumulative;
}

time_t getPrevTime() {
    return prevTime;
}

void setPrevTime(time_t newVal) {
    prevTime = newVal;
}

double getRealTimeSum() {
    return realTimeSum;
}

void setRealTimeSum(double newVal) {
    realTimeSum = newVal;
}

double getRealTimeCount() {
    return realTimeCount;
}

void incRealTimeCount() {
    realTimeCount++;
}

time_t getInterruptTime() {

  return interruptTime;

}

void handleInterrupt() {

  //reset if the button pushed for more than 5 seconds
  while(digitalRead(CONTROL_BUTTON) == 0) {
    if(now() - interruptTime >= 3) {
      wificonfig_reset();
      ESP.reset();
    }
  }

  //reset interrupt status
  interruptTime = 0;

}