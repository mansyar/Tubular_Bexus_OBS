#ifndef SIMULATION_H
#define SIMULATION_H

#include "sensorManager.h"

struct pressureSimulation{
  int simulationTime[8];
  int pressureSim[nrPressSensors][8];
  int temperatureSim[nrTempSensors][8];
  int airflowSim[nrAirFSensors][8];
  int humSim[nrHumidSensors][8];
};

bool checkSimulationOrNot();
pressureSimulation getSimulationData ();


#endif