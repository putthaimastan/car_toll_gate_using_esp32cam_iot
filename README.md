# ESP32-CAM Car Gate System
 # 🚗 ESP32-CAM Car Gate System

An automatic car gate system using **ESP32-CAM**, **IR sensors**, **SG90 Servo Motor**, and integrated with **Firebase Realtime Database** to track car entries and exits with timestamp.

---

## Features

- Capture photo of incoming cars using ESP32-CAM
- Log entry/exit status and timestamps to Firebase
- Control gate via SG90 servo motor
- Detect vehicle presence using IR sensors

---

## How to Use

1. Open the `.ino` sketch file in **Arduino IDE**
2. Install the required libraries:
   - Firebase ESP Client
   - NTPClient
   - Base64
   - ESP32Servo
3. Replace the following placeholders in the code:
   ```cpp
   #define WIFI_SSID "REPLACE_WITH_YOUR_WIFI_NAME"
   #define WIFI_PASSWORD "REPLACE_WITH_YOUR_WIFI_PASSWORD"
   #define API_KEY "REPLACE_WITH_YOUR_FIREBASE_API_KEY"
   #define DATABASE_URL "REPLACE_WITH_YOUR_FIREBASE_DATABASE_URL"
