# Autonomous-spatial-measurement-system
Autonomous room-measuring robot based on ESP32, ToF laser sensor, IMU, and web GUI for accurate spatial mapping.

# Autonomous Spatial Measurement System 🚗📏

An autonomous mobile robot designed for precise indoor room measurement and spatial mapping, replacing manual measuring tapes and reducing error margins in architectural planning.

## 📌 Features
- **Autonomous Wall-Following:** Navigates around room perimeter using IR tracking and real-time path adjustments.
- **High-Precision Laser Measurement:** Uses ToF (Time-of-Flight) laser sensors for millimeter-accurate height and distance sampling.
- **Loop Closure & Angle Detection:** Integrates IMU (Gyroscope/Accelerometer) and optical wheel encoders to track position, detect corners, and calculate total perimeter, area, and volume.
- **Web Interface (WebGUI):** Built-in HTTP server hosted on ESP32 for wireless control, status monitoring, and report downloading.
- **Onboard Display & Logging:** Real-time data visualization on a 4.0" TFT LCD display, with RTC time-stamped logs stored on a MicroSD card.

## 🛠️ Hardware Components
- **Microcontroller:** ESP32-DevKitC (Dual-core 32-bit, Wi-Fi/BLE)
- **Distance & Navigation Sensors:**
  - ToF Laser Range Sensor (VL53L1X)
  - IMU Motion Sensor (MPU6050)
  - IR Proximity / Wall Tracker Sensors
  - Optical Wheel Encoders
- **Drive System:** DC Motors driven by DRV8833 Motor Driver
- **Display & Interface:** 4.0" TFT SPI LCD Display, Buzzer Indicator
- **Real-Time Clock:** DS3231 RTC Module

## 💻 Software & Libraries
- Written in **C++** via **Arduino IDE**
- Key Libraries: `Adafruit_VL53L1X`, `Adafruit_MPU6050`, `RTClib`, `Adafruit_ST7796S`, `WebServer`, `WiFi`

## 📸 Media & Documentation
- **Project Report:** Full technical documentation and circuit schematics are available in the project documentation.