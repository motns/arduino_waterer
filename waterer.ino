/**
MIT License
Copyright (c) 2023 Adam Borocz
*/
// #include <Arduino_MKRIoTCarrier.h>
// #include "waterLevelSensor.h"
// #include "moistureSensor.h"
#include "watererController.h"

// TODO - IoT cloud stuff
int waterLevelPct = 0;
int moisture1Pct = 0;
int moisture2Pct = 0;
bool pump1On = false;
bool pump2On = false;
int pump1SecsSinceLastRun = 0;
int pump2SecsSinceLastRun = 0;

// MKRIoTCarrier carrier;
//WaterLevelSensor waterLevelSensor;
// MoistureSensor moistureSensor(A5);
WatererController watererController(
  &waterLevelPct,
  &moisture1Pct,
  &moisture2Pct,
  &pump1On,
  &pump2On,
  &pump1SecsSinceLastRun,
  &pump2SecsSinceLastRun,
  true
);

void setup() {
  Serial.begin(9600);
  watererController.init();
  Serial.println("Init done");
}

void loop() {
  watererController.run();
}
