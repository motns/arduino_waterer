/**
MIT License
Copyright (c) 2023 Adam Borocz
*/
#ifndef GUARD_E513344B_9711_4050_80AC_C4B095D3F0B3_H_
#define GUARD_E513344B_9711_4050_80AC_C4B095D3F0B3_H_
# include <stdexcept>

class MoistureSensor {
 public:
  /**
   * @param pin The analogue pin the sensor is attached to
   * @param maxValue Used to specify the highest reading received when sensor is dry (calibrate via experimentation)
   * @param minValue Used to specify the lowest reading received when sensor is submerged in water (calibrate via experimentation)
  */
  explicit MoistureSensor(int pin, int maxValue = 795, int minValue = 285) {  // Default values set by testing Moisture Sensor 2.0
    // if (maxValue < 0 || 1023 < maxValue) {
    //   throw std::invalid_argument(
    //     "maxValue needs to be in the 0 to 1023 range");
    // }
    this->minValue = minValue;
    this->maxValue = maxValue;
    this->highestVal = 0;
    this->lowestVal = 1023;

    this->pin = pin;
  }

  int getValue() {
    const int rawVal = analogRead(this->pin);
    delay(10);  // To let the AD converter recover

    if (rawVal < this->lowestVal) {
      this->lowestVal = rawVal;
      // Serial.print(F("New low observed for moisture val: "));
      // Serial.println(this->lowestVal);
    }

    if (this->highestVal < rawVal) {
      this->highestVal = rawVal;
      // Serial.print(F("New high observed for moisture val: "));
      // Serial.println(this->highestVal);
    }

    const int mappedVal = map(rawVal, this->maxValue, this->minValue, 0, 100);

    if (100 < mappedVal) {
      // Serial.print(F("Raw moisture value exceeded expected minimum of "));
      // Serial.print(this->minValue);
      // Serial.print(F(": "));
      // Serial.println(rawVal);
      return 100;
    } else if (mappedVal < 0) {
      // Serial.print(F("Raw moisture value exceeded expected maximum of "));
      // Serial.print(this->maxValue);
      // Serial.print(F(": "));
      // Serial.println(rawVal);
      return 0;
    } else {
      return mappedVal;
    }
  }

 private:
  int pin;
  int maxValue;  // The expected highest reading - used to map to percentage
  int minValue;  // The expected lowest reading - used to map to a percentage
  int highestVal;  // The highest reading observed since startup
  int lowestVal;  // The lowest reading observed since startup
};

#endif  // GUARD_E513344B_9711_4050_80AC_C4B095D3F0B3_H_
