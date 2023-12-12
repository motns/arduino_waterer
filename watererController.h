/**
MIT License
Copyright (c) 2023 Adam Borocz
*/
#ifndef GUARD_00B3831E_140F_4C74_963F_AA13AC6A6446_H_
#define GUARD_00B3831E_140F_4C74_963F_AA13AC6A6446_H_

#include <Arduino_MKRIoTCarrier.h>
#include "waterLevelSensor.h"
#include "moistureSensor.h"

const String PROGMEM okStr = "OK";
const String PROGMEM warnStr = "WARN";
const String PROGMEM critStr = "CRIT";

// 0 is lowest, whereas 200 seems to be what's set
// when CARRIER_CASE = false
const int PROGMEM buttonSensitivity = 125;

const touchButtons PROGMEM prevButton = TOUCH0;
const touchButtons PROGMEM nextButton = TOUCH4;
const touchButtons PROGMEM actionButton = TOUCH2;

const int PROGMEM moisture1Pin = A5;
const int PROGMEM moisture2Pin = A6;

const long PROGMEM pump1CheckInterval = 60000;  // How frequently to check whether pump needs to be triggered
const int PROGMEM moisture1TriggerThreshold = 50;  // Only auto trigger pump below this threshold
const long PROGMEM pump2CheckInterval = 67000;
const int PROGMEM moisture2TriggerThreshold = 50;

const int PROGMEM warnPercentage = 50;
const int PROGMEM criticalPercentage = 25;

class WatererController {
 public:
  explicit WatererController(
    int* waterLevelPct,
    int* moisture1Pct,
    int* moisture2Pct,
    bool* pump1On,
    bool* pump2On,
    long* pump1MsSinceLastRun,
    long* pump2MsSinceLastRun,
    int maxWaterLevel = 80,  // At what sensor percentage is the water tank full (find via testing)
    int pumpFlowRate = 8,  // In mL/s (find via testing)
    bool isPlant2Enabled = true  // Set to false if only one plant is being monitored/watered
  ) : carrier(), moisture1Sensor(moisture1Pin), moisture2Sensor(moisture2Pin) {
    this->waterLevelPct = waterLevelPct;
    this->moisture1Pct = moisture1Pct;
    this->moisture2Pct = moisture2Pct;
    this->pump1On = pump1On;
    prevPump1On = false;
    pump1LastRunMs = 0;
    this->pump1MsSinceLastRun = pump1MsSinceLastRun;
    this->pump2On = pump2On;
    prevPump2On = false;
    pump2LastRunMs = 0;
    this->pump2MsSinceLastRun = pump2MsSinceLastRun;
    this->maxWaterLevel = maxWaterLevel;
    this->pumpFlowRate = pumpFlowRate;
    this->isPlant2Enabled = isPlant2Enabled;
    currentMillis = millis();
    prevMillis = 0;
  }

  void init() {
    // This is needed here, otherwise our button sensitivity configuration
    // is overwritten in `Arduino_MKRIoTCarrier.begin()`
    CARRIER_CASE = true;
    delay(1500);

    // Despite what the documentation tells us, these need to be set
    // *before* `begin()` is called...
    carrier.Buttons.updateConfig(buttonSensitivity);
    carrier.Buttons.updateConfig(buttonSensitivity, prevButton);
    carrier.Buttons.updateConfig(buttonSensitivity + 40, nextButton);  // Seems more sensitive than others
    carrier.Buttons.updateConfig(buttonSensitivity, actionButton);

    if (!carrier.begin()) {
      Serial.println("Carrier not connected, check connections");
      while (1) {}
    }

    waterLevelSensor.init();

    carrier.display.setRotation(0);
    carrier.display.setTextWrap(true);
  }

  // Business logic for loop() function
  void run() {
    currentMillis = millis();
    updateSensorValues();
    updateSystemStatus();

    carrier.Buttons.update();
    if (carrier.Buttons.onTouchDown(nextButton)) {
      currentScreen = getNextScreen();
      singleBeep();
    } else if (carrier.Buttons.onTouchDown(prevButton)) {
      currentScreen = getPreviousScreen();
      singleBeep();
    } else if (carrier.Buttons.onTouchDown(actionButton)) {
      actionButtonPressed();
    }

    triggerPump();  // Scheduled turn on for pumps
    updatePumps();  // Put this here because state might change due to button press
    drawScreen();

    updatePrevValues();
    delay(10);
  }

 private:
  MKRIoTCarrier carrier;

  int currentMillis;
  int prevMillis;

  int* waterLevelPct;
  int prevWaterLevelPct;
  int* moisture1Pct;
  int prevMoisture1Pct;
  int* moisture2Pct;
  int prevMoisture2Pct;

  bool* pump1On;
  bool prevPump1On;
  long pump1LastRunMs;
  long* pump1MsSinceLastRun;
  bool* pump2On;
  bool prevPump2On;
  long pump2LastRunMs;
  long* pump2MsSinceLastRun;
  long pump1OffAtMillis;
  long pump2OffAtMillis;

  int maxWaterLevel;
  int pumpFlowRate;

  bool isPlant2Enabled;

  enum SystemStatus {
    unknown,  // TODO - Implement optional instead?
    green,
    warn,
    critical
  };

  SystemStatus systemStatus = green;  // Current system status
  SystemStatus prevSystemStatus;  // System status in previous iteration

  MoistureSensor moisture1Sensor;
  MoistureSensor moisture2Sensor;
  WaterLevelSensor waterLevelSensor;

  enum WatererScreen {
    blankScreen,  // TODO - Implement optional instead?
    statusScreen,
    waterLevelScreen,
    moisture1Screen,
    pump1Screen,
    moisture2Screen,
    pump2Screen
  };

  enum Plant {
    plantOne,
    plantTwo
  };

  WatererScreen currentScreen = statusScreen;  // What screen is currently open
  WatererScreen prevScreen = blankScreen;  // What screen was shown in the previous iteration

  void singleBeep() {
    carrier.Buzzer.beep(2637, 100);
  }

  WatererScreen getNextScreen() {
    switch (currentScreen) {
      case statusScreen: return waterLevelScreen;
      case waterLevelScreen: return moisture1Screen;
      case moisture1Screen: return pump1Screen;
      case pump1Screen:
        if (isPlant2Enabled) {
          return moisture2Screen;
        } else {
          return statusScreen;
        }
      case moisture2Screen: return pump2Screen;
      case pump2Screen: return statusScreen;
    }
  }

  WatererScreen getPreviousScreen() {
    switch (currentScreen) {
      case statusScreen:
        if (isPlant2Enabled) {
          return pump2Screen;
        } else {
          return pump1Screen;
        }
      case waterLevelScreen: return statusScreen;
      case moisture1Screen: return waterLevelScreen;
      case pump1Screen: return moisture1Screen;
      case moisture2Screen: return pump1Screen;
      case pump2Screen: return moisture2Screen;
    }
  }

  void actionButtonPressed() {
    if (currentScreen == pump1Screen) {
      singleBeep();
      togglePump(plantOne);
    } else if (currentScreen == pump2Screen) {
      singleBeep();
      togglePump(plantTwo);
    } else {  // No action
      singleBeep();  // Make into triple beep
    }
  }

  void togglePump(Plant plant) {
    if (plant == plantOne) {
      *pump1On = !*pump1On;
    } else {
      *pump2On = !*pump2On;
    }
  }

  void triggerPump() {
    if (
      !*pump1On
      && *moisture1Pct < moisture1TriggerThreshold
      && pump1CheckInterval < *pump1MsSinceLastRun
    ) {
      *pump1On = true;
    }

    if (
      !*pump2On
      && *moisture2Pct < moisture2TriggerThreshold
      && pump2CheckInterval < *pump2MsSinceLastRun
    ) {
      *pump2On = true;
    }
  }

  void drawScreen() {
    switch(currentScreen) {
      case statusScreen:
        drawStatusScreen();
        break;
      case waterLevelScreen:
        drawWaterLevelScreen();
        break;
      case moisture1Screen:
        drawMoistureScreen(plantOne);
        break;
      case pump1Screen:
        drawPumpScreen(plantOne);
        break;
      case moisture2Screen:
        drawMoistureScreen(plantTwo);
        break;
      case pump2Screen:
        drawPumpScreen(plantTwo);
        break;
      default:
        break;
    }
  }

  void writeCenteredText(const String &str, int size, int x = 120, int y = 120) {
    carrier.display.setTextSize(size);
    carrier.display.setTextWrap(true);
    int16_t x1, y1;
    uint16_t w, h;
    carrier.display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    carrier.display.setCursor(x - w / 2, y - h / 2);
    carrier.display.println(str);
  }

  void drawStatusScreen() {
    if (prevScreen != currentScreen) {
      carrier.display.fillScreen(ST77XX_BLACK);
      const int colour = colorForStatus(systemStatus);

      // Make the circle thicker
      carrier.display.drawCircle(120, 120, 112, colour);
      carrier.display.drawCircle(120, 120, 111, colour);
      carrier.display.drawCircle(120, 120, 110, colour);

      carrier.display.setTextColor(colour);
      carrier.display.setTextSize(4);
      writeCenteredText(textForStatus(systemStatus), 4);
    } else {
      if (prevSystemStatus != systemStatus) {
        const int colour = colorForStatus(systemStatus);

        // Draw over other circle
        carrier.display.drawCircle(120, 120, 110, colour);
        carrier.display.drawCircle(120, 120, 111, colour);
        carrier.display.drawCircle(120, 120, 112, colour);

        // Overwrite last status
        carrier.display.setTextColor(ST77XX_BLACK);
        writeCenteredText(textForStatus(prevSystemStatus), 4);
        carrier.display.setTextColor(colour);
        writeCenteredText(textForStatus(systemStatus), 4);
      }
    }
  }

  int colorForStatus(SystemStatus status) {
    switch (status) {
      case green: return ST77XX_GREEN;
      case warn: return ST77XX_YELLOW;
      case critical: return ST77XX_RED;
    }
  }

  // TODO - Learn about what "const" actually does... :D
  const String& textForStatus(SystemStatus status) {
    switch (status) {
      case green: return okStr;
      case warn: return warnStr;
      case critical: return critStr;
    }
  }

  void drawProgressCircle(int progress, int colour = ST77XX_WHITE) {
    if (progress = 0) return;

    const int quadrants = map(progress, 1, 100, 1, 4);

    // TODO - Ask Michael whether there's a better way of doing this...
    // TODO - Also, this seems to be currently broken, so we'll stick to a normal circle...
    uint8_t mask = 0;
    for (int i = 1; i <= quadrants; i++) {
      mask += pow(2, i - 1);
    }
    carrier.display.drawCircleHelper(120, 120, 110, mask, colour);
    carrier.display.drawCircleHelper(120, 120, 111, mask, colour);
    carrier.display.drawCircleHelper(120, 120, 112, mask, colour);
  }

  void drawWaterLevelScreen() {
    if (prevScreen != currentScreen) {
      carrier.display.fillScreen(ST77XX_BLACK);
      drawPercentageData(F("Water Level"), *waterLevelPct, colourForPercentage(*waterLevelPct));
    } else {
      if (prevWaterLevelPct != *waterLevelPct) {
        // Draw over previous one
        drawPercentageData(F("Water Level"), prevWaterLevelPct, ST77XX_BLACK);
        drawPercentageData(F("Water Level"), *waterLevelPct, colourForPercentage(*waterLevelPct));
      }
    }
  }

  void drawMoistureScreen(Plant plant) {
    if (prevScreen != currentScreen) {  // Draw new from scratch
      carrier.display.fillScreen(ST77XX_BLACK);
      if (plant == plantOne) {
        drawPercentageData(F("Moisture 1"), *moisture1Pct, colourForPercentage(*moisture1Pct));
      } else {
        drawPercentageData(F("Moisture 2"), *moisture2Pct, colourForPercentage(*moisture2Pct));
      }
    } else {  // Redraw current
      if (plant == plantOne) {
        if (prevMoisture1Pct != *moisture1Pct) {
          drawPercentageData(F("Moisture 1"), prevMoisture1Pct, ST77XX_BLACK);
          drawPercentageData(F("Moisture 1"), *moisture1Pct, colourForPercentage(*moisture1Pct));
        }
      }

      if (plant == plantTwo) {
        if (prevMoisture2Pct != *moisture2Pct) {
          drawPercentageData(F("Moisture 2"), prevMoisture2Pct, ST77XX_BLACK);
          drawPercentageData(F("Moisture 2"), *moisture2Pct, colourForPercentage(*moisture2Pct));
        }
      }
    }
  }

  void drawPercentageData(String label, int pct, int colour) {
    carrier.display.setTextColor(colour);
    writeCenteredText(label, 2, 120, 100);
    writeCenteredText(String(pct) + "%", 3, 120, 145);
    //drawProgressCircle(pct, colour);
    carrier.display.drawCircle(120, 120, 110, colour);
    carrier.display.drawCircle(120, 120, 111, colour);
    carrier.display.drawCircle(120, 120, 112, colour);
  }

  int colourForPercentage(int pct) {
    if (pct <= criticalPercentage) {
      return ST77XX_RED;
    } else if (pct <= warnPercentage) {
      return ST77XX_YELLOW;
    } else {
      return ST77XX_BLUE;
    }
  }

  void drawPumpScreen(Plant plant) {
    if (prevScreen != currentScreen) {  // Fresh screen
      carrier.display.fillScreen(ST77XX_BLACK);
      const String label = (plant == plantOne) ? "Pump 1" : "Pump 2";
      const bool pumpOn = (plant == plantOne) ? *pump1On : *pump2On;
      const int colour = (pumpOn) ? ST77XX_BLUE : ST77XX_WHITE;

      if (pumpOn) {
        const long pumpOffAtMillis = (plant == plantOne) ? pump1OffAtMillis : pump2OffAtMillis;
        const long secondsRemaining = (pumpOffAtMillis - currentMillis) / 1000;
        drawCountdown(label, secondsRemaining, colour);
      } else {
        drawCountdown(label, 0, colour);
      }
    } else {  // Redraw partial screen
      const bool pumpOn = (plant == plantOne) ? *pump1On : *pump2On;
      const bool prevPumpOn = (plant == plantOne) ? prevPump1On : prevPump2On;

      if (pumpOn) {
        if (!prevPumpOn) {
          carrier.display.fillScreen(ST77XX_BLACK);
        }

        const String label = (plant == plantOne) ? "Pump 1" : "Pump 2";
        const long pumpOffAtMillis = (plant == plantOne) ? pump1OffAtMillis : pump2OffAtMillis;
        const long secondsRemaining = (pumpOffAtMillis - currentMillis) / 1000;
        const long prevSecondsRemaining = (pumpOffAtMillis - prevMillis) / 1000;

        if (prevSecondsRemaining != secondsRemaining) {
          drawCountdown(label, prevSecondsRemaining, ST77XX_BLACK);
          drawCountdown(label, secondsRemaining, ST77XX_BLUE);
        }
      } else {
        if (prevPumpOn) {
          const String label = (plant == plantOne) ? "Pump 1" : "Pump 2";
          // If we're transitioning out of a countdown, it's easier to just wipe the screen and start again
          carrier.display.fillScreen(ST77XX_BLACK);
          drawCountdown(label, 0, ST77XX_WHITE);
        }
      }
    }
  }

  void drawCountdown(String label, int remainingSeconds, int colour) {  // TODO - include total seconds
    carrier.display.setTextColor(colour);
    writeCenteredText(label, 2, 120, 100);
    if (remainingSeconds == 0) {
      writeCenteredText("Off", 3, 120, 145);
    } else {
      writeCenteredText(String(remainingSeconds), 3, 120, 145);
    }
    //drawProgressCircle(pct, colour);
    carrier.display.drawCircle(120, 120, 110, colour);
    carrier.display.drawCircle(120, 120, 111, colour);
    carrier.display.drawCircle(120, 120, 112, colour);
  }

  void updateSensorValues() {
    updateWaterLevel();
    updateMoisturePct(plantOne);
    if (isPlant2Enabled) {
      updateMoisturePct(plantTwo);
    }
  }

  void updateWaterLevel() {
    const int rawPct = waterLevelSensor.getLevelPercentage();
    int mappedPct = map(rawPct, 0, maxWaterLevel, 0, 100);

    if (100 < mappedPct) {
      Serial.print(F("Water level sensor exceeded expected maximum of "));
      Serial.print(maxWaterLevel);
      Serial.println(F(": "));
      Serial.println(rawPct);
      mappedPct = 100;  // TODO - Overflow warning?
    }

    *waterLevelPct = mappedPct;
  }

  void updateMoisturePct(Plant plant) {
    if (plant == plantOne) {
      *moisture1Pct = moisture1Sensor.getValue();
    } else {
      *moisture2Pct = moisture2Sensor.getValue();
    }
  }

  void updateSystemStatus() {
    if (*waterLevelPct <= criticalPercentage) {
      systemStatus = critical;
    } else if (*waterLevelPct <= warnPercentage) {
      systemStatus = warn;
    } else {
      systemStatus = green;
    }
  }

  void updatePumps() {
    updatePump(plantOne);
    updatePump(plantTwo);
    *pump1MsSinceLastRun = currentMillis - pump1LastRunMs;
    *pump2MsSinceLastRun = currentMillis - pump2LastRunMs;
  }

  void updatePump(Plant plant) {
    bool* pumpOn = (plant == plantOne) ? pump1On : pump2On;
    bool* prevPumpOn = (plant == plantOne) ? &prevPump1On : &prevPump2On;
    long* pumpOffAtMillis = (plant == plantOne) ? &pump1OffAtMillis : &pump2OffAtMillis;

    if (*pumpOn) {
      if (!*prevPumpOn) {  // Previously off - set timer
        *pumpOffAtMillis = currentMillis + 5000;
      } else {  // Previously on - check timer
        if (*pumpOffAtMillis <= currentMillis) {  // Timer passed - turn off
          *pumpOn = false;
        }
      }
    }

    // Update relay status
    if (*pumpOn) {
      // It seems like the lights are the other way around - light is *on* when relay is Open...
      if (plant == plantOne) {
        carrier.Relay1.open();
        pump1LastRunMs = currentMillis;
      } else {
        carrier.Relay2.open();
        pump2LastRunMs = currentMillis;
      }
    } else {
      (plant == plantOne) ? carrier.Relay1.close() : carrier.Relay2.close();
    }
  }

  void updatePrevValues() {
    prevMillis = currentMillis;
    prevSystemStatus = systemStatus;
    prevWaterLevelPct = *waterLevelPct;
    prevMoisture1Pct = *moisture1Pct;
    prevMoisture2Pct = *moisture2Pct;
    prevPump1On = *pump1On;
    prevPump2On = *pump2On;
    prevScreen = currentScreen;
  }
};

#endif  // GUARD_00B3831E_140F_4C74_963F_AA13AC6A6446_H_