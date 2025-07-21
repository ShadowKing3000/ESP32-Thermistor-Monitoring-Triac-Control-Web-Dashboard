#include <Wire.h>
#include <PCF8574.h>
#include <math.h>

// Thermistor configuration
const int analogPin = 12;
const float seriesResistor = 10000.0;
const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float betaCoefficient = 3950.0;
const float adcMax = 4095.0;
const float vcc = 3.3;

// PCF8574 I2C setup
PCF8574 pcf8574(0x20);

int a;

void setup() {
  Wire.begin(21, 22);              // ESP32 I2C pins
  Serial.begin(115200);

  pcf8574.begin();

  for (int i = 0; i < 8; i++) {
    pcf8574.pinMode(i, OUTPUT);
    pcf8574.digitalWrite(i, HIGH);  // Default HIGH (relay off)
  }
}

void loop() {
  // Read ADC and calculate temperature (°C)
  int adcValue = analogRead(analogPin);
  float resistance = seriesResistor * ((float)adcValue / (adcMax - adcValue));
  
  float steinhart = log(resistance / nominalResistance);
  steinhart = 1.0 / (steinhart / betaCoefficient + 1.0 / (nominalTemperature + 273.15));
  float temperatureC = steinhart - 273.15;

  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | Temp: ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  // Read serial command and toggle relays
  if (Serial.available()) {
    a = Serial.parseInt();
    Serial.print("Command: ");
    Serial.println(a);

    if (a >= 1 && a <= 16) {
      int pin = (a - 1) / 2;
      int state = (a % 2 == 1) ? LOW : HIGH;
      pcf8574.digitalWrite(pin, state);
    }
  }

  delay(1000);
}
