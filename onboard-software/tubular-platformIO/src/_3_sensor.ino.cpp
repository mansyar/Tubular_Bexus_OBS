/* Name: _3_sensor.ino.cpp
 * Purpose: To interface with the sensors, take samples and store them.
 * Project: Tubular-Bexus.
 * Authors: Tubular-Bexus software group.
*/
#ifndef UNIT_TEST
#include "_3_sensor.h"
#include <SD.h>
#include <Wire.h>
// #include <M2M_LM75A.h>
// #include <MS5611.h>
// #include <MS5xxx.h>
// #include <AWM43300V.h>
#include <AWM5102VN.h>
#include <HDC2010.h> //humidity sensor lib
#include <Ethernet2.h>
#include <MS5607.h>
#include <FreeRTOS_ARM.h>
#include "Series3500.h"
#include "DS1631.h"


#include "ethernet.h"
#include "_1_init.h"
#include "_4_heater.h"
#include "_5_asc.h"
#include "_6_telemetry.h"
#include "_8_monitor.h"
#include "simulation.h"

extern ethernet ethernet;
extern RTCDue rtc;
DS1631 DS1631;

MS5607 pressSensor1(pressSensorPin1); //Ambient Pressure Sensor
MS5607 pressSensor2(pressSensorPin2); //Ambient Pressure Sensor
MS5607 pressSensor3(pressSensorPin3); //ValveCenter Pressure Sensor
MS5607 pressSensor4(pressSensorPin7); //ValveCenter Pressure Sensor
Series3500 pressSensorStatic(staticPressPin); //Static pressure sensor pin
HDC2010 humSensor(hdcADDR);

// Sd2Card card;

#define TEMP_ADDR (0x90 >> 1) 
//I2C start adress


AWM5102VN afSensor(airFsensorPin);

pressureSimulation sim_data;
 
bool simulationOrNot;
extern SemaphoreHandle_t sem;
int samplingRate = 1000;
int connectionTimeout = 180;
int tcReceived;
float medianPressureAmbient;

File dataLog;
File root;
File nextFile;
boolean eof = false;
int count=0;
String file = "";
String fileNum = "";

/*Must reset sensors before reading PROM on startup*/
void initPressureSensor()
{
  if(!simulationOrNot){
    
    //delay(3); 
    pressSensor1.PROMread(pressSensorPin1);
    pressSensor2.PROMread(pressSensorPin2);
    pressSensor3.PROMread(pressSensorPin3);
    pressSensor4.PROMread(pressSensorPin7);
  }
  else{
    pressSensor4.PROMread(pressSensorPin7);
  }
}

void resetPressureSensor()
{
   if(!simulationOrNot){
   pressSensor1.reset_sequence(pressSensorPin1);
   pressSensor2.reset_sequence(pressSensorPin2);
   pressSensor3.reset_sequence(pressSensorPin3);
   pressSensor4.reset_sequence(pressSensorPin7);
   }
   else{
     pressSensor4.reset_sequence(pressSensorPin7);
   }

}

void pressSensorread()
{
  if(!simulationOrNot){// If Not in simullation do this.
        //Read pressure for all pressure sensor(s).
        
        pressSensor4.PROMread(pressSensorPin7); 
        //Start Convertion (of pressure) for all pressure sensor(s).
        pressSensor1.convertionD1(4, pressSensorPin1);
        pressSensor2.convertionD1(4, pressSensorPin2);
        pressSensor3.convertionD1(4, pressSensorPin3);
        pressSensor4.convertionD1(4, pressSensorPin7);

        delay(15);

        pressSensor1.ADCpress = pressSensor1.readADC(pressSensorPin1);
        pressSensor2.ADCpress = pressSensor2.readADC(pressSensorPin2);
        pressSensor3.ADCpress = pressSensor3.readADC(pressSensorPin3);
        pressSensor4.ADCpress = pressSensor4.readADC(pressSensorPin7);

        delay(10);

         //Start Convertion (of temperature) for all pressure sensor(s).
        pressSensor1.convertionD2(4, pressSensorPin1);
        pressSensor2.convertionD2(4, pressSensorPin2);
        pressSensor3.convertionD2(4, pressSensorPin3);
        pressSensor4.convertionD2(4, pressSensorPin7);

        delay(15);

        //Read temperature for all pressure sensor(s).
        pressSensor1.ADCtemp = pressSensor1.readADC(pressSensorPin1);
        pressSensor2.ADCtemp = pressSensor2.readADC(pressSensorPin2);
        pressSensor3.ADCtemp = pressSensor3.readADC(pressSensorPin3);
        pressSensor4.ADCtemp = pressSensor4.readADC(pressSensorPin7);

        //Calculating the correct temperature and pressure.
        pressSensor1.ADC_calc(pressSensor1.ADCpress, pressSensor1.ADCtemp);
        pressSensor2.ADC_calc(pressSensor2.ADCpress, pressSensor2.ADCtemp);
        pressSensor3.ADC_calc(pressSensor3.ADCpress, pressSensor3.ADCtemp);
        pressSensor4.ADC_calc(pressSensor4.ADCpress, pressSensor4.ADCtemp);  

        pressSensor4.reset_sequence(pressSensorPin7);
        //delay(5);
         
  }
  else{
    pressSensor4.convertionD1(4, pressSensorPin7);
    delay(15);
    pressSensor4.ADCpress = pressSensor4.readADC(pressSensorPin7);
    delay(10);
    pressSensor4.convertionD2(4, pressSensorPin7);
    delay(15);
    pressSensor4.ADCtemp = pressSensor4.readADC(pressSensorPin7);
    pressSensor4.ADC_calc(pressSensor4.ADCpress, pressSensor4.ADCtemp);
  }
}



void initHumSensor()
{
  
  //Serial.println("I'm at initHumSensor");  
   // Initialize I2C communication
   humSensor.begin();
    
   // Begin with a device reset
   //humSensor.reset();

   // Configure Measurements
   humSensor.setMeasurementMode(TEMP_AND_HUMID);  // Set measurements to temperature and humidity
   humSensor.setRate(FIVE_HZ);                     // Set measurement frequency to 1 Hz
   humSensor.setTempRes(FOURTEEN_BIT);
   humSensor.setHumidRes(FOURTEEN_BIT);

   //begin measuring
   humSensor.triggerMeasurement();
}

void savingDataToSD(float temperatureData[], float humData[], float pressData[], float afData[])
{
  Serial.println("I'm at savingDataToSD");

  /*  If current file exceds a certain size
  //    create new file with a new name. 
  */
  // delay(150);
  // Serial.print("EOF"); Serial.println(eof);
  root = SD.open("/");
  // Serial.println("SD OPEN.");
  if(root) {
    root.close();
     root = SD.open("/log/");

    while(!eof)
     {
        count++;
      // Serial.print("Count: ");  Serial.println(count);
      nextFile=root.openNextFile();
     
       if(!nextFile)
       {
         eof=true;
       }
     
     if(eof)
     {
     fileNum = String(count);
     file = "/log/log"+fileNum+".txt";
    //  Serial.print("Name of file: "); Serial.println(file);
     }
     
     nextFile.close();
   }
   root.close();
   char fileName[file.length()+1];
   file.toCharArray(fileName, sizeof(fileName));
   dataLog = SD.open(fileName, FILE_WRITE);
  //  Serial.print("Current file size: "); Serial.println(dataLog.size());
   if(dataLog.size()>(200*1024)) {
     count = 0;
     eof = false;
    //  Serial.println("EOF. Set to false.");
     
   }
   if(!dataLog) {
     Serial.print("Failed to open: "); Serial.println(fileName);
   }
  //  Serial.print("file.length(): "); Serial.println(file.length());
   



  String dataString = "";
  //File dataLog = SD.open("datalog.txt", FILE_WRITE);
  if (dataLog)
  {
     if(dataLog.size()<10)
     {
       String headers = "TIME, TEMPERATURE, PRESSURE, AIRFLOW, HUMIDITY,";
       dataLog.print(headers);
       dataLog.println();

     }
     // Serial.println("I'm at dataLog");
    dataString += String(rtc.getHours());
    dataString += ":";
    dataString += String(rtc.getMinutes());
    dataString += ":";
    dataString += String(rtc.getSeconds());
    dataString += ",";

    dataLog.print(dataString);
    //Serial.println(dataString);
    dataString = "";

    // Serial.println("I'm at dataLog");
    
    for (int i = 0; i < nrTempSensors; i++)
    {
      dataString += String(temperatureData[i]);
      dataString += ",";
      if (i == (nrTempSensors - 1))
      {
        dataString += "";
      }
      //Serial.println(temperatureData[i]);
    }
    dataLog.print(dataString);

    dataString = "";
    for (int i = 0; i < nrPressSensors; i++)
    {
      dataString += String(pressData[i]);
      dataString += ",";
      if (i == (nrPressSensors - 1))
      {
        dataString += "";
      }
    }
    dataLog.print(dataString);
    
    dataString = "";
    for (int i = 0; i < nrAirFSensors; i++)
    {
      dataString += String(afData[i]);
      //dataString += analogRead(A0)*3.3/1024;
      dataString += ",";
      if (i == (nrAirFSensors - 1))
      {
        dataString += "";
      }
    }
    dataLog.print(dataString);

    dataString = "";
    for (int i = 0; i < nrHumidSensors; i++)
    {
      dataString += String(humData[i]);
      dataString += ",";
      if (i == (nrHumidSensors - 1))
      {
        dataString += "";
      }
    }
    dataLog.print(dataString);

    dataString = "";
    dataString += String(digitalRead(pumpPin)); dataString += ",";
    dataString += String(digitalRead(valve1)); dataString += ",";
    dataString += String(digitalRead(valve2)); dataString += ",";
    dataString += String(digitalRead(valve3)); dataString += ",";
    dataString += String(digitalRead(valve4)); dataString += ",";
    dataString += String(digitalRead(valve5)); dataString += ",";
    dataString += String(digitalRead(valve6)); dataString += ",";
    dataString += String(digitalRead(valve7)); dataString += ",";
    dataString += String(digitalRead(valve8)); dataString += ",";
    dataString += String(digitalRead(valve9)); dataString += ",";
    dataString += String(digitalRead(valve10)); dataString += ",";
    dataString += String(digitalRead(flushValve)); dataString += ",";
    dataString += String(digitalRead(CACvalve)); dataString += ",";
    dataString += String(digitalRead(htr1_pin)); dataString += ",";
    dataString += String(digitalRead(htr2_pin)); dataString += ",";
    
    dataLog.print(dataString);

    dataLog.println();
    dataLog.close();

    // Serial.println("Exiting dataLog");
    //Serial.println(dataString);
  }
 
  }
  else
  {
    Serial.println("Failed to open root");
    // Serial.print("sdPin: ");Serial.println(digitalRead(sdPin));
    SD.end();

    
  }
  // Serial.println("leaving SavingData");
  // delay(10);
}

void setSamplingRate(int curSamplingRate)
{
   xSemaphoreTake(sem, portMAX_DELAY);
   samplingRate = curSamplingRate;
   xSemaphoreGive(sem);
}

int getSamplingRate(void)
{
   int tempSamplingRate;
   xSemaphoreTake(sem, portMAX_DELAY);
   tempSamplingRate = samplingRate;
   xSemaphoreGive(sem);
   return tempSamplingRate;
   
}

void writeData(float curMeasurements [], int type)
{
   xSemaphoreTake(sem, portMAX_DELAY);
   writeDataToSensorBuffers(curMeasurements, type);
   xSemaphoreGive(sem);
}

std::vector<float> readData(int type)
{
  std::vector<float> dataFromBuffer;
  xSemaphoreTake(sem, portMAX_DELAY);
  dataFromBuffer = readDataFromSensorBuffers(type);
  xSemaphoreGive(sem);
  return dataFromBuffer;
}

void initTempSensors()
{
  // Serial.println("I'm at initTempSensors");
  Wire.begin();
  for(int i=0;i<=(nrTempSensors-1);i++) 
  {
    DS1631.initDS1631(TEMP_ADDR+i);
  }

  //"The special one", is baked into a pressure sensor.
  //tempSensor9.begin();

}


void sampler(void *pvParameters)
{
   (void) pvParameters;

   //float tempPressure[nrPressSensors];
   //int count=0;
   float pressDifference = 0;
   float curPressureMeasurement[nrPressSensors] = {0};
   float curTemperatureMeasurement[nrTempSensors] = {0};
   float curHumMeasurement[nrHumidSensors] = {0};
   float curAFMeasurement[nrAirFSensors] = {0};
   //float medianPressureAmbient;
  //  float medianPressureAmbient;
   int currSamplingRate;

   static BaseType_t xHigherPriorityTaskWoken;
   TickType_t xLastWakeTime;
   xLastWakeTime = xTaskGetTickCount ();
   

   while(1)
   {
      // Serial.println("I'm at sensor periodic");
      //Serial.print("Time: ");
      //Serial.println(rtc.getSeconds());
      xHigherPriorityTaskWoken = pdFALSE;
      uint8_t currMode = getMode();

      if (!simulationOrNot)
      {
        pressSensorread();

        /*Read pressure from sensors*/
        curPressureMeasurement[0] = pressSensor1.getPres()/float(100);
        curPressureMeasurement[1] = pressSensor2.getPres()/float(100);   
        curPressureMeasurement[2] = pressSensor3.getPres()/float(100);
        // curPressureMeasurement[3] = pressSensor4.getPres()/float(100);
        curPressureMeasurement[3] = pressSensorStatic.getPress();

        // Serial.print("Pressure sensor1: "); Serial.println(curPressureMeasurement[0]);

        /*read humidity from HDC*/
        //float temperatureHDC = humSensor.readTemp();
        //curHumMeasurement[0] = humSensor.readHumidity();


        /*Read temperature from sensors*/
        // Serial.println("Temp reading");
        for(uint8_t i=0;i<(nrTempSensors-1);i++) {
            curTemperatureMeasurement[i] = DS1631.getTemperature(TEMP_ADDR+i);
        }
        // Serial.print("Temp sensor value: "); Serial.println(curTemperatureMeasurement[7]);

        curTemperatureMeasurement[8] = pressSensor4.getTemp()/float(100);
        curPressureMeasurement[4] = pressSensor4.getPres()/float(100);
        // Serial.println("Leaving temp reading");
        //Serial.println("leaving Temp reading");
      


        /*Read airflow from sensor*/
        curAFMeasurement[0] = afSensor.getAF();
        //Serial.println("I'm at normal");
        //Serial.println(analogRead(A0)*3.3/1024);
        //Serial.println(curAFMeasurement[0]);

      }
      else
      {
        // Serial.println("I'm at simulation");
        /*Simulation*/
        int secondsNow = getCurrentTime();
        for (int seq = 0; seq < (simulationPoints-1); seq++)
        {
          if (secondsNow > sim_data.simulationTime[seq] && secondsNow < sim_data.simulationTime[seq+1])
          {
            /*Read pressure from sensors*/
            for (int l = 0; l < nrPressSensors; l++)
            {
              if (l<4)
              {
                curPressureMeasurement[l] = sim_data.pressureSim[l][seq];
              }
              else
              {
                curPressureMeasurement[l] = 0;
              }
            }
          }
        }
        if (secondsNow > sim_data.simulationTime[simulationPoints-1])
          {
            /*Read pressure from sensors*/
            for (int l = 0; l < nrPressSensors; l++)
            {
              if (l<4)
              {
                curPressureMeasurement[l] = sim_data.pressureSim[l][simulationPoints-1];
              }
              else
              {
                curPressureMeasurement[l] = 0;
              }
            }
          }
          pressSensorread();
          //curPressureMeasurement[3] = pressSensorStatic.getPress();
          curPressureMeasurement[4] = pressSensor4.getPres()/float(100);
          Serial.print("Pressure : "); Serial.println(curPressureMeasurement[4]);
          curAFMeasurement[0] = afSensor.getAF();

          /*Read temperature from sensors*/
        // Serial.println("Temp reading");
        for(uint8_t i=0;i<(nrTempSensors-1);i++) {
            curTemperatureMeasurement[i] = DS1631.getTemperature(TEMP_ADDR+i);
            // Serial.print("Temperature reading: "); Serial.println(curTemperatureMeasurement[i]);
        }
        curTemperatureMeasurement[8] = pressSensor4.getTemp()/float(100);

        
      }
        Serial.println("Saving data");

        /*Save pressure measurements*/
        writeData(curPressureMeasurement, 2);

        /*Save temperature measurements*/
        writeData(curTemperatureMeasurement, 0);

        /*Save humidity measurements*/
        writeData(curHumMeasurement, 1);

        /*Save humidity measurements*/
        writeData(curAFMeasurement, 3);
      
        /*Save all data to SD*/
        savingDataToSD(curTemperatureMeasurement, curHumMeasurement, curPressureMeasurement, curAFMeasurement);
        // delay(150);
        Serial.println("Left SavingData");
      
      int orderMedian[3]={0};
      if (curPressureMeasurement[0]>=curPressureMeasurement[1])
      {
        orderMedian[0]=+1;
      } else
      {
        orderMedian[1]=+1;
      }

      if (curPressureMeasurement[0]>=curPressureMeasurement[2])
      {
        orderMedian[0]=+1;
      } else
      {
        orderMedian[2]=+1;
      }

      if (curPressureMeasurement[1]>=curPressureMeasurement[2])
      {
        orderMedian[1]=+1;
      } else
      {
        orderMedian[2]=+1;
      }

      for (int medianFind = 0; medianFind<=2; medianFind++){
        if (orderMedian[medianFind]==1) {
          medianPressureAmbient = curPressureMeasurement[medianFind];
          
          break;
        }
      }

      //Vacuum chamber purpose
     // medianPressureAmbient = curPressureMeasurement[4];

      /*Calculating Pressure Difference*/
      pressDifference = calculatingPressureDifference(medianPressureAmbient);
      // Serial.println("Left press diff");
      // Serial.println(pressDifference);

      /*Change mode if the condition is satisfied*/
      if (pressDifference<pressDifferentThresholdneg && getMode() != manual && getMode() != safeMode && getMode() != normalDescent)
      {
        setMode(normalAscent);
      }
      else if (pressDifference>pressDifferentThresholdpos && getMode() != manual && getMode() != safeMode)
      {
          setMode(normalDescent);
      }
      else if (currMode==normalDescent && medianPressureAmbient>=safeModeThreshold)
      {
        setMode(safeMode);
      }
      // Serial.println("Begin Transmit");
      /*Transmit telemetry to GS*/
      transmit();
      // Serial.println("Transmit Done");
      
      /*Listen to GS*/
      EthernetClient client = ethernet.checkClientAvailibility();
      if(client.available()>0)
      {
         tcReceived = getCurrentTime();
         xSemaphoreGiveFromISR(semPeriodic, &xHigherPriorityTaskWoken );
      }
      // Serial.print("PHYCFGR : "); Serial.println(w5500.getPHYCFGR());
      if((tcReceived + connectionTimeout) < getCurrentTime() && getMode() == manual)
      {
         client.stop();
         setMode(standbyMode); //Complicates testing
        //  digitalWrite(CACvalve, LOW);
        valvesControl(11, closeState);
        pumpControl(closeState);
        for (int sd = 1;sd <= 6; sd++)
        {
          valvesControl(sd, closeState);
        }
      }

      //Serial.println("Listen for GS");
      /*Check current sampling rate*/
      currSamplingRate = getSamplingRate();
      flagPost(0);
      Serial.println("I'm leaving sensor periodic");
      vTaskDelayUntil(&xLastWakeTime, (currSamplingRate / portTICK_PERIOD_MS) );
   }
}

void initSampler()
{
    //Serial.println("I'm at initSampler");
    xTaskCreate(
      sampler
      ,  (const portCHAR *)"Sampler"   // A name just for humans
      ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
      ,  NULL
      ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      ,  NULL );
}

void initSensor()
{
   //Serial.println("I'm at initSensor");
   simulationOrNot = checkSimulationOrNot(); 
   if (simulationOrNot)
   {
      sim_data = getSimulationData();
      Serial.println("Succes getting simulation data");
   }
   
  resetPressureSensor(); 
  initHumSensor(); 
  initTempSensors();
  initPressureSensor(); 
  
  initSampler();
}
#endif
