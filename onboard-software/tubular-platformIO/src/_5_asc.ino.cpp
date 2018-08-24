/*
 * Name: asc object
 * Purpose: To read the pressure sensor from buffer
 * and accordingly activate sampling logic.
 * Project: Tubular-Bexus.
 * Authors: Tubular-Bexus software group.
*/
#ifndef UNIT_TEST
#include <SD.h>
#include "_4_heater.h"
#include "_5_asc.h"
#include "ascLogic.h"
#include "_8_monitor.h"


float ascParameter[totalBagNumber*2];
extern RTCDue rtc;
int secondsOpen;
int flushStartTime;
int pumpStartTime;
int valveBagStartTime;
// int bagFillingTime [] = {43, 46, 53, 50, 47, 41};
int bagFillingTime [] = {4, 4, 4, 4, 4, 4};


std::vector<float> getASCParam(int bag)
{
  std::vector<float> dummyParameter(2);
  xSemaphoreTake(sem, portMAX_DELAY);
  dummyParameter[0] = ascParameter[(bag*2)-2];
  dummyParameter[1] = ascParameter[(bag*2)-1];
  xSemaphoreGive(sem);
  return dummyParameter;
}

std::vector<float> processInitialAscParameters(char scParameters[])
{
  //Serial.println("I'm at processInitialAscParameters");
  //Serial.println(String(scParameters));
  std::vector<float> newASCParameter(totalBagNumber*2);
  
  int i = 0; int z = 0;
  //int sizeParam = sizeof(scParameters)/sizeof(byte);
  while(z<totalBagNumber*2)
  {
    int k = 0;
    char buf[6] = {0};
    while(1)
    {
      if (scParameters[i] == ',')
      {
        i++;
        break;
      }
      buf[k] = scParameters[i];
      
      i++; k++;
    }
    newASCParameter[z] = atof(buf);
    //Serial.println(newASCParameter[z]);
    z++;
  }
  return newASCParameter;
}

void initAscParameters()
{
  
  File dataParam = SD.open("asc.txt", FILE_READ);
  if (dataParam)
  {
    char scParameters[dataParam.size()];
    int i = 0;
    while (dataParam.available())
    {
        scParameters[i] = dataParam.read();
        i++;
    }
    float newParameter[totalBagNumber*2];
    std::vector<float> newParameterV = processInitialAscParameters(scParameters);
    for (int scP = 0; scP < totalBagNumber*2; scP++)
    {
        newParameter[scP] = newParameterV[scP];
    }
    dataParam.close();
    setASCParameter(newParameter);
  }
  else
  {
    float backupAscParam[] = {80,70,51.8,41.8,76.2,86.2,98,108,136,146,188.3,198.3};
    setASCParameter(backupAscParam);
    Serial.println("Failed to open asc.txt");
  }
  

}

void setASCParameter(float newParameter[])
{
  xSemaphoreTake(sem, portMAX_DELAY);
  for (int i = 0; i < totalBagNumber*2; i++)
  {
    ascParameter[i]=newParameter[i];
  }
  xSemaphoreGive(sem);
}

void initValvesControl()
{
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(valve3, OUTPUT);
  pinMode(valve4, OUTPUT);
  pinMode(valve5, OUTPUT);
  pinMode(valve6, OUTPUT);
  pinMode(valve7, OUTPUT);
  pinMode(valve8, OUTPUT);
  pinMode(valve9, OUTPUT);
  pinMode(valve10, OUTPUT);
  pinMode(flushValve, OUTPUT);
  pinMode(CACvalve, OUTPUT);
}

void pumpControl(int cond)
{
  // Serial.println("Pump control taking sem");
  xSemaphoreTake(sem, portMAX_DELAY);
  // Serial.println("Entering pump control");
  // switch (cond){
  //   case 0:
  //   digitalWrite(pumpPin, LOW);
  //   break;

  //   case 1:
  //   heaterControl(0,0); //Turn off heaters when pump is on.
  //   digitalWrite(pumpPin, HIGH);
  //   break;
  // }
  digitalWrite(pumpPin, cond);
  xSemaphoreGive(sem);
  // Serial.println("Leaving pump control");
}

void valvesControl(int valve, int cond)
{
  // Serial.println("valves control taking sem");
  xSemaphoreTake(sem, portMAX_DELAY);
  // Serial.println("Entering valves control");
  if (cond==1)
  {
    switch (valve)
    {
      case 1:
      digitalWrite(valve1, HIGH);
      break;

      case 2:
      digitalWrite(valve2, HIGH);
      break;

      case 3:
      digitalWrite(valve3, HIGH);
      break;

      case 4:
      digitalWrite(valve4, HIGH);
      break;

      case 5:
      digitalWrite(valve5, HIGH);
      break;

      case 6:
      digitalWrite(valve6, HIGH);
      break;

      case 7:
      digitalWrite(valve7, HIGH);
      break;

      case 8:
      digitalWrite(valve8, HIGH);
      break;

      case 9:
      digitalWrite(valve9, HIGH);
      break;

      case 10:
      digitalWrite(valve10, HIGH);
      break;

      case 11:
      digitalWrite(flushValve, HIGH);
      break;
  }
  }
  else {
    switch (valve){
    case 1:
    digitalWrite(valve1, LOW);
    break;

    case 2:
    digitalWrite(valve2, LOW);
    break;

    case 3:
    digitalWrite(valve3, LOW);
    break;

    case 4:
    digitalWrite(valve4, LOW);
    break;

    case 5:
    digitalWrite(valve5, LOW);
    break;

    case 6:
    digitalWrite(valve6, LOW);
    break;

    case 7:
    digitalWrite(valve7, LOW);
    break;

    case 8:
    digitalWrite(valve8, LOW);
    break;

    case 9:
    digitalWrite(valve9, LOW);
    break;

    case 10:
    digitalWrite(valve10, LOW);
    break;

    case 11:
    digitalWrite(flushValve, LOW);
    break;

    default:
    break;
  }
  
  }
  xSemaphoreGive(sem);
  // Serial.println("Leaving valves control");
}

int getCurrentTime()
{
  int secondsNow = rtc.getSeconds() + (rtc.getMinutes()*60) + (rtc.getHours()*3600);
  return secondsNow;
}

int ascentSequence(float meanPressureAmbient, float ascParam[], int bagcounter)
{
  digitalWrite(CACvalve, HIGH);
  
  // int secondsNow = getCurrentTime();
  int valveBag = digitalRead(valve1 + bagcounter - 1);
  int valveFlush = digitalRead(flushValve);
  int pumpState = digitalRead(pumpPin);
  
  if (ascentSamplingLogic(meanPressureAmbient, ascParam))
  {
    if (valveBag == closeState)
    {
      if (pumpState == closeState)
      {
        pumpControl(openState);
        pumpStartTime = getCurrentTime();
      }
      else
      {
        if (getCurrentTime() > pumpStartTime+1)
        {
          if (valveFlush == closeState)
          {
            valvesControl(11, openState);
            flushStartTime = getCurrentTime();
          }
          else
          {
            if (getCurrentTime() > (flushStartTime+flushingTime))
            {
              valvesControl(11, closeState);
              valvesControl(bagcounter, openState);
              valveBagStartTime = getCurrentTime();
            }
          }
        }
      }
    }
    else
    {
      if (getCurrentTime() > (valveBagStartTime+bagFillingTime[bagcounter-1]))
      {
        valvesControl(bagcounter, closeState);
        pumpControl(closeState);
        bagcounter++;
      }
    }
  }
  
  return bagcounter;
  
}

int descentSequence(float meanPressureAmbient, float ascParam[], int bagcounter)
{
  int valveBag = digitalRead(valve1 + bagcounter - 1);
  int valveFlush = digitalRead(flushValve);
  int pumpState = digitalRead(pumpPin);
  
  if (descentSamplingLogic(meanPressureAmbient, ascParam))
  {
    if (valveBag == closeState)
    {
      if (pumpState == closeState)
      {
        pumpControl(openState);
        pumpStartTime = getCurrentTime();
      }
      else
      {
        if (getCurrentTime() > pumpStartTime+1)
        {
          if (valveFlush == closeState)
          {
            valvesControl(11, openState);
            flushStartTime = getCurrentTime();
          }
          else
          {
            if (getCurrentTime() > (flushStartTime+flushingTime))
            {
              valvesControl(11, closeState);
              valvesControl(bagcounter, openState);
              valveBagStartTime = getCurrentTime();
            }
          }
        }
      }
    }
    else
    {
      if (getCurrentTime() > (valveBagStartTime+bagFillingTime[bagcounter-1]))
      {
        valvesControl(bagcounter, closeState);
        pumpControl(closeState);
        bagcounter++;
      }
    }
  }
  else
  {
    if (valveBag == openState)
    {
      valvesControl(bagcounter, closeState);
      pumpControl(closeState);
      bagcounter++;
    }
  }
  
  return bagcounter;
}

void reading(void *pvParameters)
{
   (void) pvParameters;
   std::vector<float> dummyParam(2);
   float ascParam[2];
   int bagcounter = 1;
   std::vector<float> currPressure(nrPressSensors);
   float meanPressureAmbient;
   uint8_t currMode;
   
   TickType_t xLastWakeTime;
   xLastWakeTime = xTaskGetTickCount ();

   while(1)
   {
      Serial.println("I'm at asc periodic");
      currMode = getMode();
     
     dummyParam = getASCParam(bagcounter);
     ascParam[0] = dummyParam[0]; // lower limit
     ascParam[1] = dummyParam[1]; // upper limit
     currPressure = readData(2);

     /*Calculating mean pressure from several pressure sensors*/
     meanPressureAmbient = (currPressure[0]+currPressure[1])/2;

     switch (currMode){
     /*Standby*/
     case standbyMode:
     break;
     
     /*Normal - Ascent*/
     case normalAscent:
     if (ascentOrDescent(ascParam))
     {
       if (bagcounter <= totalBagNumber)
       {
         bagcounter = ascentSequence(meanPressureAmbient, ascParam, bagcounter);
       }
     }
     break;
     
     /*Normal - Descent*/
     case normalDescent:
     if (!ascentOrDescent(ascParam))
     {
       if (bagcounter <= totalBagNumber)
       {
         bagcounter = descentSequence(meanPressureAmbient, ascParam, bagcounter);
       }
     }
     break;
     
     /*SAFE*/
     case safeMode:
     digitalWrite(CACvalve, LOW);
     pumpControl(closeState);
     for (int sd = 1;sd <= 6; sd++)
     {
       valvesControl(sd, closeState);
     }
     break;

     case manual:
     
     break;
   }
   flagPost(2);
   Serial.println("I'm leaving asc periodic");
   vTaskDelayUntil(&xLastWakeTime, (800 / portTICK_PERIOD_MS) );
   }
}

void initReading()
{
  //Serial.println("Im at initReading");
  xTaskCreate(
    reading
    ,  (const portCHAR *) "reading"   // Name
    ,  2048  // This stack size 
    ,  NULL
    ,  2  // Priority
    ,  NULL );
}

void initPumpControl()
{
  pinMode(pumpPin, OUTPUT);
}

void initASC()
{
  // Serial.println("Im at initAsc");
  initAscParameters();
  initPumpControl();
  initValvesControl();
  initReading();
  
}
#endif