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
long pump1MsSinceLastRun = 0;
long pump2MsSinceLastRun = 0;

// MKRIoTCarrier carrier;
//WaterLevelSensor waterLevelSensor;
// MoistureSensor moistureSensor(A5);
WatererController watererController(
  &waterLevelPct,
  &moisture1Pct,
  &moisture2Pct,
  &pump1On,
  &pump2On,
  &pump1MsSinceLastRun,
  &pump2MsSinceLastRun,
  true
);

const int maxWaterLevel = 80;

int lastVal = 0;

void setup() {
  Serial.begin(9600);
  watererController.init();

  // CARRIER_CASE = true;
  // delay(1500);

  // if (!carrier.begin()) {
  //   Serial.println("Carrier not connected, check connections");
  //   while (1) {}
  // }

  // carrier.display.setRotation(0);
  // carrier.display.setTextWrap(true);
  // carrier.display.setTextSize(5);
  // carrier.display.fillScreen(ST77XX_WHITE);
  Serial.println("Init done");
}

void loop() {
  watererController.run();

  // Serial.print("Pump1 ms since run: ");
  // Serial.println(pump1MsSinceLastRun);
  // Serial.print("Pump2 ms since run: ");
  // Serial.println(pump2MsSinceLastRun);

  // Serial.print("Water level: ");
  // Serial.println(waterLevelPct);
  // Serial.print("Moisture1: ");
  // Serial.println(moisture1Pct);
  // Serial.print("Moisture2: ");
  // Serial.println(moisture2Pct);

  // int rawWaterLevel = waterLevelSensor.getLevelPercentage();
  // int waterLevel = map(rawWaterLevel, 0, maxWaterLevel, 0, 100);
  // Serial.print("Water level: ");
  // Serial.println(waterLevel);

  // int rawVal = moistureSensor.getValue();
  // Serial.print("Level: ");
  // Serial.println(rawVal);

  // if (lastVal != rawVal) {
  //   carrier.display.setCursor(110, 100);
  //   carrier.display.setTextColor(ST77XX_WHITE);
  //   carrier.display.println(lastVal);
  //   carrier.display.setCursor(110, 100);
  //   carrier.display.setTextColor(ST77XX_RED);
  //   carrier.display.println(rawVal);
  //   lastVal = rawVal;
  // }

  // delay(100);
}
