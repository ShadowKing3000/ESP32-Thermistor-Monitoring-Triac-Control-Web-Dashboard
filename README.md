# 🔌 ESP32 Thermistor Monitoring & Triac Control Web Dashboard

This project uses an **ESP32**, **PCF8574 I/O expander**, and **thermistor** to create a web-based dashboard that:

* 📈 Reads real-time temperature using a 10k NTC thermistor
* 🔌 Controls up to **8 triac channels** (e.g. AC loads) via I2C
* 🌐 Serves a live web interface for monitoring and switching
* ⚡ Measures current using an **ACS712 sensor**

---

## 🛠️ Hardware Used

| Component         | Description                             |
| ----------------- | --------------------------------------- |
| ESP32             | Microcontroller with Wi-Fi              |
| PCF8574           | 8-bit I/O expander via I2C              |
| Thermistor (10kΩ) | NTC type for temperature sensing        |
| ACS712            | Current sensor module (5A/20A/30A)      |
| BT136/MOC3021     | For triac triggering (external circuit) |

---

## 🧰 Features

* 📡 Web server hosted on ESP32
* 🌡️ Temperature reading using B-parameter equation
* ⚙️ Individual triac ON/OFF control from web
* ⚡ Live current measurement (non-RMS)
* 📱 Mobile-friendly HTML interface (Chart + Buttons)

---

## 📋 API Endpoints

| Endpoint           | Method | Description                               |
| ------------------ | ------ | ----------------------------------------- |
| `/`                | GET    | Returns web interface                     |
| `/api/temperature` | GET    | Returns latest temperature                |
| `/api/states`      | GET    | Returns all triac states                  |
| `/api/control`     | POST   | JSON: `{"pin": 0-7, "state": true/false}` |

---

## 🌐 Web Interface Preview

> Displays:
>
> * 📊 Real-time temperature chart
> * 🟢 Toggle switches for each triac
> * ⚡ Current draw in Amps

---

## 🔧 Setup Instructions

1. Wire up ESP32 with PCF8574, thermistor, ACS712
2. Flash the code using Arduino IDE (or PlatformIO)
3. Change your WiFi credentials in the code:

   ```cpp
   const char* ssid = "YourSSID";
   const char* password = "YourPassword";
   ```
4. Open serial monitor to get ESP32 IP
5. Visit `http://<ESP32-IP>` from your browser

---

## 📊 Data Visualization (Optional)

You can log temperature data over time using CSV + Python:

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('500w.csv')
df.columns = ['Time', 'Temperature']
plt.plot(df['Time'], df['Temperature'])
plt.xlabel('Time (s)')
plt.ylabel('Temperature (°C)')
plt.title('Temperature vs Time')
plt.grid()
plt.show()
```

---
