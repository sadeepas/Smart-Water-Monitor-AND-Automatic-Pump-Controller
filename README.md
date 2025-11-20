# Smart-Water-Monitor-AND-Automatic-Pump-Controller
A robust, standalone IoT solution built on the ESP32 platform. This system monitors water levels using ultrasonic sensors, controls a water pump automatically based on configurable thresholds, and serves a modern, real-time web dashboard directly from the device

Here is a professional and comprehensive `README.md` file for your GitHub repository.

It includes the pinout configuration based on your code, installation instructions (including the critical SPIFFS data upload step), and a feature breakdown.

***

## 🌟 Features

*   **Standalone Operation:** Creates its own Wi-Fi Access Point (`WaterMonitorAP`). No internet or home router required.
*   **Real-Time Dashboard:** Serves a beautiful, responsive web interface with:
    *   Live animated water level gauge.
    *   Real-time history chart.
    *   Pump status and sensor indicators.
    *   **WebSocket** technology for instant data updates (no page refreshing needed).
*   **Automatic Control:** Intelligent logic to turn the pump ON/OFF based on water levels.
*   **Safety Mechanisms:**
    *   **Hysteresis:** Prevents pump rapid cycling/chattering.
    *   **Dry-Run Protection:** Shuts off pump if water isn't rising after 30 seconds.
    *   **Max Runtime Cutoff:** Hard stop after 5 minutes to prevent overheating.
    *   **Overflow Protection:** Dedicated top-level sensor interrupt.
*   **Remote Configuration:** Change tank dimensions and trigger thresholds directly from the web UI.

## 🛠️ Hardware Requirements

*   **ESP32 Development Board** (e.g., ESP32 DevKit V1)
*   **HC-SR04** Ultrasonic Sensor
*   **Relay Module** (5V, to control the pump)
*   **Float Switch** (Optional, for top-level safety)
*   **Water Pump**
*   Jumper wires and Breadboard/PCB

## 🔌 Pinout Configuration

Based on the provided firmware, wire your components as follows:

| Component | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **Ultrasonic Trigger** | `GPIO 5` | Sends the sound pulse |
| **Ultrasonic Echo** | `GPIO 18` | Receives the sound pulse |
| **Pump Relay** | `GPIO 2` | Controls the pump (Active High) |
| **Top Sensor** | `GPIO 4` | Safety float switch (Input Pullup) |
| **Power** | `VIN / GND` | Power the board appropriately |

*> **Note:** You can change these pin definitions in the `CONSTANTS` section of the `.ino` file.*

## 💻 Software Dependencies

You need the **Arduino IDE** with the ESP32 board package installed. Additionally, install the following libraries via the Arduino Library Manager:

1.  **ESPAsyncWebServer** (by Hristo Gochkov)
2.  **AsyncTCP** (by Hristo Gochkov)
3.  **ArduinoJson** (by Benoit Blanchon)

*> Note: If you cannot find ESPAsyncWebServer in the standard library manager, you may need to download it from GitHub and install it as a .zip library.*

## 🚀 Installation & Setup

### 1. Prepare the File System (SPIFFS)
The web dashboard (`index.html`) is stored in the ESP32's flash memory.
1.  In your Arduino project folder, create a folder named `data`.
2.  Place your `index.html` file inside the `data` folder.
3.  **Important:** You must upload this file to the ESP32.
    *   *Arduino IDE v1.8:* Use the "ESP32 Sketch Data Upload" plugin.
    *   *Arduino IDE v2.0+:* Use a compatible partition uploader or file management tool.

### 2. Flash the Firmware
1.  Open the `.ino` file in Arduino IDE.
2.  Select your board (e.g., DOIT ESP32 DEVKIT V1).
3.  Connect your ESP32 via USB.
4.  Click **Upload**.

## 📱 Usage

1.  Power on the ESP32.
2.  On your phone or laptop, search for Wi-Fi networks.
3.  Connect to:
    *   **SSID:** `WaterMonitorAP`
    *   **Password:** `pump12345`
4.  Open a web browser and navigate to: `http://192.168.4.1`
5.  The dashboard should load immediately.

## ⚙️ Configuration

The system uses the following default logic, which can be adjusted via the Web UI or in the code:

*   **Tank Height:** 100cm (Default)
*   **Pump ON:** When water is below 20%.
*   **Pump OFF:** When water is above 30% (20% + 10% Hysteresis).
*   **Safety Timeout:** 5 Minutes.

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!
1.  Fork the project.
2.  Create your feature branch (`git checkout -b feature/AmazingFeature`).
3.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
4.  Push to the branch (`git push origin feature/AmazingFeature`).
5.  Open a Pull Request.

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.

---

*Developed by [Sadeepa Lakshan](https://github.com/sadeepas)*
