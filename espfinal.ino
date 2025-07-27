#include <Wire.h>
#include <PCF8574.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <math.h>

// WiFi credentials
const char* ssid = "Automation";
const char* password = "9898008310";

// Thermistor config
const int analogPin = 34;
const float seriesResistor = 10000.0;
const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float betaCoefficient = 3950.0;
const float adcMax = 4095.0;
const float vcc = 3.3;

// ACS712 config
const int ACS_PIN = 35;
const float mVperAmp = 66.0;
const int NUM_SAMPLES = 2000;
int ACS_OFFSET_ADC = 0;
float currentRMS_global = 0.0;

// I/O Expander (PCF8574)
PCF8574 pcf8574(0x20);

// Web server
WebServer server(80);

// State variables
float currentTemperature = 0.0;
float currentAmps = 0.0;
bool triacStates[8] = { false };

// Web interface
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Thermistor Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .temperature-display {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            margin-bottom: 30px;
            font-size: 24px;
            font-weight: bold;
        }
        .controls {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        .triac-control {
            background-color: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            border: 1px solid #e9ecef;
            text-align: center;
        }
        .triac-control h3 {
            margin: 0 0 10px 0;
            color: #495057;
        }
        .btn {
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s ease;
        }
        .btn-on {
            background-color: #28a745;
            color: white;
        }
        .btn-on:hover {
            background-color: #218838;
        }
        .btn-off {
            background-color: #dc3545;
            color: white;
        }
        .btn-off:hover {
            background-color: #c82333;
        }
        .btn:disabled {
            background-color: #6c757d;
            cursor: not-allowed;
        }
        .status {
            margin-top: 10px;
            padding: 5px 10px;
            border-radius: 3px;
            font-size: 12px;
            font-weight: bold;
        }
        .status-on {
            background-color: #d4edda;
            color: #155724;
        }
        .status-off {
            background-color: #f8d7da;
            color: #721c24;
        }
        .last-updated {
            text-align: center;
            color: #6c757d;
            margin-top: 20px;
            font-size: 14px;
        }
        .connection-status {
            position: fixed;
            top: 10px;
            right: 10px;
            padding: 5px 10px;
            border-radius: 5px;
            font-size: 12px;
            font-weight: bold;
        }
        .connected {
            background-color: #d4edda;
            color: #155724;
        }
        .disconnected {
            background-color: #f8d7da;
            color: #721c24;
        }
    </style>
</head>
<body>
    <div class="connection-status" id="connectionStatus">Connected</div>
    <div class="container">
        <h1>ESP32 Thermistor & Triac Control</h1>
        
        <div class="temperature-display">
            <div>Current Temperature</div>
            <div id="temperature">--°C</div>
        </div>
        
        <div class="temperature-display" style="background: linear-gradient(135deg, #ff6b6b 0%, #ee5a52 100%);">
            <div>AC Current</div>
            <div id="current">-- A</div>
        </div>
        
        <div class="controls">
            <div class="triac-control">
                <h3>Triac 1</h3>
                <button class="btn btn-on" onclick="controlTriac(1, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(1, false)">OFF</button>
                <div class="status status-off" id="status1">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 2</h3>
                <button class="btn btn-on" onclick="controlTriac(2, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(2, false)">OFF</button>
                <div class="status status-off" id="status2">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 3</h3>
                <button class="btn btn-on" onclick="controlTriac(3, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(3, false)">OFF</button>
                <div class="status status-off" id="status3">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 4</h3>
                <button class="btn btn-on" onclick="controlTriac(4, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(4, false)">OFF</button>
                <div class="status status-off" id="status4">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 5</h3>
                <button class="btn btn-on" onclick="controlTriac(5, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(5, false)">OFF</button>
                <div class="status status-off" id="status5">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 6</h3>
                <button class="btn btn-on" onclick="controlTriac(6, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(6, false)">OFF</button>
                <div class="status status-off" id="status6">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 7</h3>
                <button class="btn btn-on" onclick="controlTriac(7, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(7, false)">OFF</button>
                <div class="status status-off" id="status7">OFF</div>
            </div>
            
            <div class="triac-control">
                <h3>Triac 8</h3>
                <button class="btn btn-on" onclick="controlTriac(8, true)">ON</button>
                <button class="btn btn-off" onclick="controlTriac(8, false)">OFF</button>
                <div class="status status-off" id="status8">OFF</div>
            </div>
        </div>
        
        <div class="last-updated" id="lastUpdated">Last updated: --</div>
    </div>

    <script>
        let connectionStatus = document.getElementById('connectionStatus');
        let isConnected = true;
        
        function updateConnectionStatus(connected) {
            isConnected = connected;
            if (connected) {
                connectionStatus.textContent = 'Connected';
                connectionStatus.className = 'connection-status connected';
            } else {
                connectionStatus.textContent = 'Disconnected';
                connectionStatus.className = 'connection-status disconnected';
            }
        }
        
        function updateTemperature() {
            fetch('/api/temperature')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature.toFixed(2) + '°C';
                    document.getElementById('current').textContent = data.current.toFixed(3) + ' A';
                    document.getElementById('lastUpdated').textContent = 'Last updated: ' + new Date().toLocaleTimeString();
                    updateConnectionStatus(true);
                })
                .catch(error => {
                    console.error('Error fetching data:', error);
                    updateConnectionStatus(false);
                });
        }
        
        function updateTriacStates() {
            fetch('/api/states')
                .then(response => response.json())
                .then(data => {
                    for (let i = 0; i < 8; i++) {
                        let statusElement = document.getElementById('status' + (i + 1));
                        if (data.states[i]) {
                            statusElement.textContent = 'ON';
                            statusElement.className = 'status status-on';
                        } else {
                            statusElement.textContent = 'OFF';
                            statusElement.className = 'status status-off';
                        }
                    }
                    updateConnectionStatus(true);
                })
                .catch(error => {
                    console.error('Error fetching states:', error);
                    updateConnectionStatus(false);
                });
        }
        
        function controlTriac(triacNumber, state) {
            let buttons = document.querySelectorAll('.btn');
            buttons.forEach(btn => btn.disabled = true);
            
            fetch('/api/control', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    triac: triacNumber,
                    state: state
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    updateTriacStates();
                } else {
                    alert('Failed to control triac');
                }
                buttons.forEach(btn => btn.disabled = false);
                updateConnectionStatus(true);
            })
            .catch(error => {
                console.error('Error controlling triac:', error);
                alert('Error controlling triac');
                buttons.forEach(btn => btn.disabled = false);
                updateConnectionStatus(false);
            });
        }
        
        // Update temperature every 2 seconds
        setInterval(updateTemperature, 2000);
        
        // Update triac states every 5 seconds
        setInterval(updateTriacStates, 5000);
        
        // Initial updates
        updateTemperature();
        updateTriacStates();
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Wire.begin(21, 22);
  Serial.begin(115200);

  pcf8574.begin();
  for (int i = 0; i < 8; i++) {
    pcf8574.pinMode(i, OUTPUT);
    pcf8574.digitalWrite(i, HIGH);
  }

  analogReadResolution(12);
  long sumOffset = 0;
  for (int i = 0; i < 1000; i++) {
    sumOffset += analogRead(ACS_PIN);
    delay(1);
  }
  ACS_OFFSET_ADC = sumOffset / 1000;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/api/temperature", handleTemperature);
  server.on("/api/states", handleStates);
  server.on("/api/control", HTTP_POST, handleControl);
  server.begin();
}

void loop() {
  server.handleClient();

  // Read thermistor
  int adcValue = analogRead(analogPin);
  float resistance = seriesResistor * ((float)adcValue / (adcMax - adcValue));
  float steinhart = log(resistance / nominalResistance);
  steinhart = 1.0 / (steinhart / betaCoefficient + 1.0 / (nominalTemperature + 273.15));
  currentTemperature = steinhart - 273.15;

  // Read current RMS
  long sumOfSquares = 0;
  int sampleCount = 0;
  unsigned long startTime = micros();
  while (micros() - startTime < (unsigned long)NUM_SAMPLES * 100) {
    int val = analogRead(ACS_PIN) - ACS_OFFSET_ADC;
    sumOfSquares += (long)val * val;
    sampleCount++;
  }
  float meanSquare = (float)sumOfSquares / sampleCount;
  float rmsADC = sqrt(meanSquare);
  float rmsVoltage = (rmsADC / adcMax) * vcc;
  float currentRMS = (rmsVoltage * 1000.0) / mVperAmp;
  currentRMS_global = currentRMS * 1.41 * 1.22; //1.22 is the calibration factor, and 1.41; for getting peak value

  // Serial debug (optional)
  Serial.print("Temp: ");
  Serial.print(currentTemperature);
  Serial.print(" °C | Current: ");
  Serial.println(currentRMS_global, 3);

  // Manual serial control
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
  doc["current"] = currentRMS_global;
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

      String resStr;
      serializeJson(response, resStr);
      server.send(200, "application/json", resStr);
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid triac number\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
  }
}
