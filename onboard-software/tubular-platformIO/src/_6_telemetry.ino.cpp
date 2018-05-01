/* Telemtery object
   Authors: BEXUS-TUBULAR software team

*/
#ifndef UNIT_TEST
#include <ArduinoSTL.h>
#include <vector>
#include <Ethernet2.h>
#include <EthernetUdp2.h>

#include "_6_telemetry.h"
#include "_2_mode.h"
#include "_3_sensor.h"
#include "_4_heater.h"
#include "_5_asc.h"

int16_t status=0;
unsigned int localPort = 8888;
IPAddress remote(1, 1, 1, 2);
EthernetUDP Udp;
RTCDue rtc1(XTAL);

void transmit() {
  std::vector<float>  tempData = readData(0);
  std::vector<float>  humidData = readData(1);
  std::vector<float>  pressData = readData(2);
  std::vector<float>  airFData = readData(3);

  Udp.beginPacket(remote, localPort);
  Udp.write("gs,");
  Udp.write(rtc1.unixtime());
  Udp.write(",ps,");
  Udp.write(nrPressSensors);
  for (int i = 0; i < nrPressSensors; i++) //Loop number of time there are press sensors
  {
    Udp.write(pressData[i]);
    Udp.write(",");
  }


  Udp.write("ts,");
  Udp.write(nrTempSensors);
  for (int i = 0; i < nrTempSensors; i++) //Loop number of time there are temp sensors
  {
    
    Udp.write(tempData[i]);
    Udp.write(",");
  }

  Udp.write("hs,");
  Udp.write(nrHumidSensors);
  for (int i = 0; i < nrHumidSensors; i++) //Loop number of time there are humidity sensors
  {
    Udp.write((humidData[i]));
    Udp.write(",");
  }

  Udp.write("as,");
  Udp.write(nrAirFSensors);
  for (int i = 0; i < nrAirFSensors; i++) //Loop number of time there are air flow sensors
  {
    Udp.write(airFData[i]);
    Udp.write(",");
  }


  Udp.write("st,");
  for (int i = 0; i <= (htr2_pin - pumpPin) ; i++) //Loop number of time there are humidity sensors
  {
    status = status + status + digitalRead(pumpPin + i);
             
  }
  Udp.write(status);
  Udp.write(",md,");
  Udp.write(getMode());
  Udp.endPacket();
}
#endif