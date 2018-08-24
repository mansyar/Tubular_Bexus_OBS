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


#include "ethernet.h"
#include "_1_init.h"
#include "_5_asc.h"
#include "_6_telemetry.h"
#include "_8_monitor.h"
#include "simulation.h"

extern ethernet ethernet;
extern RTCDue rtc;

MS5607 pressSensor1(pressSensorPin1); //Ambient Pressure Sensor
MS5607 pressSensor2(pressSensorPin2); //Ambient Pressure Sensor
MS5607 pressSensor3(pressSensorPin3); //ValveCenter Pressure Sensor
MS5607 pressSensor4(pressSensorPin4); //ValveCenter Pressure Sensor
HDC2010 humSensor(hdcADDR);

#define TEMP_ADDR (0x90 >> 1) 
//I2C start adress

int i2c_transmission = 5;

AWM5102VN afSensor(airFsensorPin);

pressureSimulation sim_data;
 
bool simulationOrNot;
extern SemaphoreHandle_t sem;
int samplingRate = 1000;
int connectionTimeout;

void initPressureSensor()
{
  pressSensor1.PROMread(pressSensorPin1);
  pressSensor2.PROMread(pressSensorPin2);
  pressSensor3.PROMread(pressSensorPin3);
  pressSensor4.PROMread(pressSensorPin4);
}

void resetPressureSensor()
{
   pressSensor1.reset_sequence(pressSensorPin1);
   pressSensor2.reset_sequence(pressSensorPin2);
   pressSensor3.reset_sequence(pressSensorPin3);
   pressSensor4.reset_sequence(pressSensorPin4);
}

void pressSensorread()
{
  //Start Convertion (of pressure) for all pressure sensor(s).
        pressSensor1.convertionD1(4, pressSensorPin1);
        pressSensor2.convertionD1(4, pressSensorPin2);
        pressSensor3.convertionD1(4, pressSensorPin3);
        pressSensor4.convertionD1(4, pressSensorPin4);

        delay(15);

        //Read pressure for all pressure sensor(s).
        pressSensor1.ADCpress = pressSensor1.readADC(pressSensorPin1);
        pressSensor2.ADCpress = pressSensor2.readADC(pressSensorPin2);
        pressSensor3.ADCpress = pressSensor3.readADC(pressSensorPin3);
        pressSensor4.ADCpress = pressSensor4.readADC(pressSensorPin4);

        //Start Convertion (of temperature) for all pressure sensor(s).
        pressSensor1.convertionD2(4, pressSensorPin1);
        pressSensor2.convertionD2(4, pressSensorPin2);
        pressSensor3.convertionD2(4, pressSensorPin3);
        pressSensor4.convertionD2(4, pressSensorPin4);

        delay(15);

        //Read temperature for all pressure sensor(s).
        pressSensor1.ADCtemp = pressSensor1.readADC(pressSensorPin1);
        pressSensor2.ADCtemp = pressSensor2.readADC(pressSensorPin2);
        pressSensor3.ADCtemp = pressSensor3.readADC(pressSensorPin3);
        pressSensor4.ADCtemp = pressSensor4.readADC(pressSensorPin4);

        //Calculating the correct temperature and pressure.
        pressSensor1.ADC_calc(pressSensor1.ADCpress, pressSensor1.ADCtemp);
        pressSensor2.ADC_calc(pressSensor2.ADCpress, pressSensor2.ADCtemp);
        pressSensor3.ADC_calc(pressSensor3.ADCpress, pressSensor3.ADCtemp);
        pressSensor4.ADC_calc(pressSensor4.ADCpress, pressSensor4.ADCtemp);
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
      create new file with a new name. Majority
      voting is used to decide the name of the file.
      TODO... 

  // Use tripple redundancy if a file corrupts when updating value.
  File value1 = SD.open("/values/value001.txt");
  File value2 = SD.open("/values/value002.txt");
  File value3 = SD.open("/values/value003.txt");

  if(value1.read()==value2.read()==value3.read())
  {
    //Serial.println("Hello------------^^");
  } else {
    int sum[3] = {0};  
    if(value1.read()==value2.read()) {
      // We have majority result if true  

    }

    TODO turn into function?
    if(value1.read()==value3.read()) {

      sum[0]= sum[0] + 1;  
      sum[2] = sum[2] + 1; 
    }
    if(value2.read()==value3.read()){
      sum[1] = sum[1] + 1; 
      sum[0]= sum[0] + 1; 
    }
    
    //bool test_bool = value1.read() xor value2.read();
    
  }*/

  String dataString = "";
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile)
  {
     // Serial.println("I'm at dataFile");
    dataString += String(rtc.getHours());
    dataString += ":";
    dataString += String(rtc.getMinutes());
    dataString += ":";
    dataString += String(rtc.getSeconds());
    dataString += "||";

    dataFile.print(dataString);
    //Serial.println(dataString);
    dataString = "";

    // Serial.println("I'm at dataFile");
    
    for (int i = 0; i < nrTempSensors; i++)
    {
      dataString += String(temperatureData[i]);
      dataString += ",";
      if (i == (nrTempSensors - 1))
      {
        dataString += "||";
      }
      //Serial.println(temperatureData[i]);
    }
    dataFile.print(dataString);
    dataString = "";
    for (int i = 0; i < nrHumidSensors; i++)
    {
      dataString += String(humData[i]);
      dataString += ",";
      if (i == (nrHumidSensors - 1))
      {
        dataString += "||";
      }
    }
    dataFile.print(dataString);
    dataString = "";
    for (int i = 0; i < nrPressSensors; i++)
    {
      dataString += String(pressData[i]);
      dataString += ",";
      if (i == (nrPressSensors - 1))
      {
        dataString += "||";
      }
    }
    dataFile.print(dataString);
    dataString = "";
    for (int i = 0; i < nrAirFSensors; i++)
    {
      //dataString += String(afData[i]);
      dataString += analogRead(A0)*3.3/1024;
      dataString += ",";
      if (i == (nrAirFSensors - 1))
      {
        dataString += "||";
      }
    }
    dataFile.print(dataString);
    dataFile.println();
    dataFile.close();
    // Serial.println("Exiting dataFile");
    //Serial.println(dataString);
  }
  else
  {
    Serial.println("Failed to open datalog.txt");
    SD.end();
    /*SD.begin(sdPin);
    File reconnect = SD.open("datalog.txt", FILE_WRITE);
    if (reconnect) {
      dataFile.println("Reconnected");
      Serial.println("Reconnected");
    }*/
    
  }
  Serial.println("leaving SavingData");
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
  Serial.println("I'm at initTempSensors");
  Wire.begin();
  for(int i=0;i<=(nrTempSensors-1);i++) 
  {
    
    Wire.beginTransmission(TEMP_ADDR+i);
      Wire.write(0xAC); // 0xAC : Acces Config
      Wire.write(0x0C); // Continuous conversion & 12 bits resolution
    Wire.endTransmission();

    Wire.beginTransmission(TEMP_ADDR+i);
      Wire.write((int)(0x51)); // Start Conversion
    Wire.endTransmission();
    // Serial.println(TEMP_ADDR+i);
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
   float meanPressureAmbient;
   int currSamplingRate;

   static BaseType_t xHigherPriorityTaskWoken;
   TickType_t xLastWakeTime;
   xLastWakeTime = xTaskGetTickCount ();
   

   while(1)
   {
      Serial.println("I'm at sensor periodic");
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
        curPressureMeasurement[3] = pressSensor4.getPres()/float(100);

        Serial.print("Pressure sensor1: "); Serial.println(curPressureMeasurement[0]);

        /*read humidity from HDC*/
        //float temperatureHDC = humSensor.readTemp();
        //curHumMeasurement[0] = humSensor.readHumidity();


        /*Read temperature from sensors*/
        Serial.println("Temp reading");
        float tempCon = 0;
        for(uint8_t i=0;i<(nrTempSensors-1);i++)
        {
          //Serial.print("Sensor adress: "); Serial.println(TEMP_ADDR+i);
          //tempCon = 0;
          Wire.beginTransmission(TEMP_ADDR+i);
            Wire.write((int)(0xAA));        // @AA : Temperature
          i2c_transmission = Wire.endTransmission();
          if (i2c_transmission==0) {
            Wire.requestFrom(TEMP_ADDR+i,2);        // READ 2 bytes
            Wire.available();                 // 1st byte
              char msb = Wire.read();      // receive a byte
            Wire.available();                 // 2nd byte
              char lsb = Wire.read()>>4;      // receive a byte

            // T° processing, works for 12-bits resolution
          
            float tempCon =0;

            if (msb >= 0x80) { //if sign bit is set, msben temp is negative
              tempCon =  (float)msb - 256 - (float)lsb/16;
            }
            else 
            {  
              tempCon = (float)msb+(float)lsb/16;  
            }
            // Serial.print("Sensor number:"); Serial.print(TEMP_ADDR+i); Serial.print("    Temp con: "); Serial.print(tempCon); Serial.println(" C ");
            curTemperatureMeasurement[i] = tempCon;
          }
          else {
            curTemperatureMeasurement[i] = -1000;
//<<<<<<< master
            // Serial.print("Error at: "); Serial.println(i);
//=======
            //Serial.print("Error at: "); Serial.println(i);
//>>>>>>> sdcard-new-file-creation
           }
        }
        Serial.println("Leaving temp reading");
        //Serial.println("leaving Temp reading");
      


        /*Read airflow from sensor*/
        curAFMeasurement[0] = afSensor.getAF();
        //Serial.println("I'm at normal");
        //Serial.println(analogRead(A0)*3.3/1024);
        //Serial.println(curAFMeasurement[0]);

      }
      else
      {
        Serial.println("I'm at simulation");
        /*Simulation*/
        int secondsNow = getCurrentTime();
        for (int seq = 0; seq < 7; seq++)
        {
          if (secondsNow > sim_data.simulationTime[seq] && secondsNow < sim_data.simulationTime[seq+1])
          {
            /*Read pressure from sensors*/
            for (int l = 0; l < nrPressSensors; l++)
            {
              if (l<2)
              {
                curPressureMeasurement[l] = sim_data.pressureSim[l][seq];
              }
              else
              {
                curPressureMeasurement[l] = 0;
              }
            }
            for (int l = 0; l < nrTempSensors; l++)
            {
              if (l<2)
              {
                curTemperatureMeasurement[l] = sim_data.temperatureSim[l][seq];
              }
              else
              {
                curTemperatureMeasurement[l] = 0;
              }
            }
            for (int l = 0; l < nrHumidSensors; l++)
            {
              curHumMeasurement[l] = sim_data.humSim[l][seq];
            }
            for (int l = 0; l < nrAirFSensors; l++)
            {
              curAFMeasurement[l] = sim_data.airflowSim[l][seq];
            }
          }
        }
        if (secondsNow > sim_data.simulationTime[7])
          {
            /*Read pressure from sensors*/
            for (int l = 0; l < nrPressSensors; l++)
            {
              if (l<2)
              {
                curPressureMeasurement[l] = sim_data.pressureSim[l][7];
              }
              else
              {
                curPressureMeasurement[l] = 0;
              }
            }
            for (int l = 0; l < nrTempSensors; l++)
            {
              if (l<2)
              {
                curTemperatureMeasurement[l] = sim_data.temperatureSim[l][7];
              }
              else
              {
                curTemperatureMeasurement[l] = 0;
              }
            }
            for (int l = 0; l < nrHumidSensors; l++)
            {
              curHumMeasurement[l] = sim_data.humSim[l][7];
            }
            for (int l = 0; l < nrAirFSensors; l++)
            {
              curAFMeasurement[l] = sim_data.airflowSim[l][7];
            }
          }
        
      }

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
        //Serial.println("Left SavingData");
      meanPressureAmbient = (curPressureMeasurement[0]+curPressureMeasurement[1])/2;
      // Serial.println("Left pressure mean");
      /*Calculating Pressure Difference*/
      pressDifference = calculatingPressureDifference(meanPressureAmbient);
      // Serial.println("Left press diff");
//<<<<<<< master
      // Serial.println(pressDifference);
//=======
      //Serial.println(pressDifference);
//>>>>>>> sdcard-new-file-creation
      /*Change mode if the condition is satisfied*/
      if (pressDifference<pressDifferentThresholdneg && getMode() != manual)
      {
        setMode(normalAscent);
      }
      else if (pressDifference>pressDifferentThresholdpos && getMode() != manual)
      {
          setMode(normalDescent);
      }
      else if (currMode==normalDescent && meanPressureAmbient>=safeModeThreshold)
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
         xSemaphoreGiveFromISR(semPeriodic, &xHigherPriorityTaskWoken );
      }
      Serial.print("PHYCFGR : "); Serial.println(w5500.getPHYCFGR());
      if(w5500.getPHYCFGR() == 186 && getMode() == manual)
      {
         client.stop();
         setMode(standbyMode);
      }

      //Serial.println("Listen for GS");
      /*Check current sampling rate*/
      currSamplingRate = getSamplingRate();
      flagPost(0);
      // Serial.println("I'm leaving sensor periodic");
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