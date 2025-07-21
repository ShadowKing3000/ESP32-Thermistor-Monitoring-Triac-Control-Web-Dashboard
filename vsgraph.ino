#include <Wire.h>
#include <PCF8574.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <math.h>

// WiFi credentials
const char* ssid = "Automation";
const char* password = "9898008310";

// Thermistor configuration
const int analogPin = 34;
const float seriesResistor = 10000.0;
const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float betaCoefficient = 3950.0;
const float adcMax = 4095.0;
const float vcc = 3.3;

// I/O expander
PCF8574 pcf8574(0x20);
WebServer server(80);

float currentTemperature = 0.0;
bool triacStates[8] = { false };

// HTML page (unchanged from original, skipped for brevity)
const char* htmlPage = R"rawliteral(
<!-- Omitted for clarity. Full HTML remains unchanged -->
)rawliteral";

void setup() {
  Wire.begin(21, 22);  // ESP32 I2C pins
  Serial.begin(115200);
  pcf8574.begin();

  for (int i = 0; i < 8; i++) {
    pcf8574.pinMode(i, OUTPUT);
    pcf8574.digitalWrite(i, HIGH);  // Relays off by default
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/api/temperature", handleTemperature);
  server.on("/api/states", handleStates);
  server.on("/api/control", HTTP_POST, handleControl);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  int adcValue = analogRead(analogPin);
  float resistance = seriesResistor * ((float)adcValue / (adcMax - adcValue));

  float steinhart = log(resistance / nominalResistance);
  steinhart = 1.0 / (steinhart / betaCoefficient + 1.0 / (nominalTemperature + 273.15));
  currentTemperature = steinhart - 273.15;

  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | Temp: ");
  Serial.print(currentTemperature);
  Serial.println(" °C");

  if (Serial.available()) {
    int a = Serial.parseInt();
    if (a >= 1 && a <= 16) {
      int pin = (a - 1) / 2;
      int state = (a % 2 == 1) ? LOW : HIGH;
      pcf8574.digitalWrite(pin, state);
      triacStates[pin] = (state == LOW);
    }
  }

  delay(1000);
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleTemperature() {
  StaticJsonDocument<200> doc;
  doc["temperature"] = currentTemperature;
  doc["timestamp"] = millis();

  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleStates() {
  StaticJsonDocument<300> doc;
  JsonArray states = doc.createNestedArray("states");

  for (int i = 0; i < 8; i++) {
    states.add(triacStates[i]);
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleControl() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, server.arg("plain"));

    int triacNumber = doc["triac"];
    bool state = doc["state"];

    if (triacNumber >= 1 && triacNumber <= 8) {
      int pin = triacNumber - 1;
      int pinState = state ? LOW : HIGH;
      pcf8574.digitalWrite(pin, pinState);
      triacStates[pin] = state;

      StaticJsonDocument<100> response;
      response["success"] = true;
      response["triac"] = triacNumber;
      response["state"] = state;

      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);

      Serial.printf("Web control - Triac %d set to %s\n", triacNumber, state ? "ON" : "OFF");
    } else {
      StaticJsonDocument<100> response;
      response["success"] = false;
      response["error"] = "Invalid triac number";

      String responseStr;
      serializeJson(response, responseStr);
      server.send(400, "application/json", responseStr);
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
  }
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
