/**
MIT License
Copyright (c) 2023 Adam Borocz
*/
#ifndef GUARD_4C284DA2_B6C7_4F1E_89EB_3FF54900B14F_H_
#define GUARD_4C284DA2_B6C7_4F1E_89EB_3FF54900B14F_H_
#include <Wire.h>

#define NO_TOUCH       0xFE
#define THRESHOLD      100
#define ATTINY1_HIGH_ADDR   0x78
#define ATTINY2_LOW_ADDR   0x77

class WaterLevelSensor {
 public:
  void init() {
    Wire.begin();
  }

  int getLevelPercentage() {
    int sensorvalue_min = 250;
    int sensorvalue_max = 255;
    int low_count = 0;
    int high_count = 0;

    uint32_t touch_val = 0;
    uint8_t trig_section = 0;
    low_count = 0;
    high_count = 0;
    getLow8SectionValue();
    getHigh12SectionValue();

    // Serial.println("low 8 sections value = ");
    for (int i = 0; i < 8; i++) {
      // Serial.print(low_data[i]);
      // Serial.print(".");
      if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max) {
        low_count++;
      }
      // if (low_count == 8) {
      //   Serial.print("      ");
      //   Serial.print("PASS");
      // }
    }
    // Serial.println("  ");
    // Serial.println("  ");
    // Serial.println("high 12 sections value = ");
    for (int i = 0; i < 12; i++) {
      // Serial.print(high_data[i]);
      // Serial.print(".");

      if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max) {
        high_count++;
      }
      // if (high_count == 12) {
      //   Serial.print("      ");
      //   Serial.print("PASS");
      // }
    }

    // Serial.println("  ");
    // Serial.println("  ");

    for (int i = 0 ; i < 8; i++) {
      if (low_data[i] > THRESHOLD) {
        touch_val |= 1 << i;
      }
    }
    for (int i = 0 ; i < 12; i++) {
      if (high_data[i] > THRESHOLD) {
        touch_val |= (uint32_t)1 << (8 + i);
      }
    }

    while (touch_val & 0x01) {
      trig_section++;
      touch_val >>= 1;
    }

    int waterLevel = trig_section * 5;
    // SERIAL.print("water level = ");
    // SERIAL.print(waterLevel);
    // SERIAL.println("% ");
    return waterLevel;
  }

 private:
  unsigned char low_data[8] = {0};
  unsigned char high_data[12] = {0};

  void getHigh12SectionValue(void) {
    memset(high_data, 0, sizeof(high_data));
    Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
    while (12 != Wire.available()) {}

    for (int i = 0; i < 12; i++) {
      high_data[i] = Wire.read();
    }
    delay(10);
  }

  void getLow8SectionValue(void) {
    memset(low_data, 0, sizeof(low_data));
    Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
    while (8 != Wire.available()) {}

    for (int i = 0; i < 8 ; i++) {
      low_data[i] = Wire.read();  // receive a byte as character
    }
    delay(10);
  }
};

#endif  // GUARD_4C284DA2_B6C7_4F1E_89EB_3FF54900B14F_H_
