# Plant Monitor for ESP32-2432S028 (Cheap Yellow Display)

This project is a monitoring application for the **ESP32-2432S028** (commonly known as the Cheap Yellow Display). It creates a tabbed user interface on the integrated screen to display real-time environmental data for various plants.

The data is retrieved via **MQTT** from a Home Assistant server (or any MQTT broker) based on the sensor configuration.

## 🚀 Features
- **Tabbed Interface**: One tab per plant for easy navigation.
- **Real-time Updates**: Subscribes to MQTT topics for instant data refreshes.
- **Touch Control**: Switch between plant views by touching the top tabs.
- **CYD Optimized**: Pre-configured with the correct pinout and drivers for the ESP32-2432S028 board.

## 🛠 Hardware Requirements
- **Board**: ESP32-2432S028 (CYD)
- **Display**: 2.8" ILI9341 SPI TFT
- **Touch**: XPT2046 Touch Controller

## 📁 Project Structure
- `platformio.ini`: PlatformIO configuration. Contains the critical `build_flags` that define the SPI pins and display drivers for the CYD hardware.
- `src/main.cpp`: The main application logic, including the MQTT client, touch handling, and UI drawing functions.
- `src/config_default.h`: A template file for your secret credentials.
- `src/config.h`: (User created) Your actual WiFi and MQTT credentials.

## ⚙️ Setup & Usage

### 1. Installation
1. Install **VS Code** and the **PlatformIO IDE** extension.
2. Open the project folder in VS Code.

### 2. Configuration
The application requires your network and MQTT details to function. For security, these are kept in a separate file:
1. Locate `src/config_default.h`.
2. **Copy** it and rename the copy to `src/config.h`.
3. Open `src/config.h` and fill in your details:
   - `ssid`: Your WiFi network name.
   - `password`: Your WiFi password.
   - `mqtt_server`: The IP address of your MQTT broker (e.g., Home Assistant).
   - `mqtt_user` / `mqtt_pass`: Your MQTT authentication credentials.

### 3. Deployment
1. Connect your CYD board to your computer via USB.
2. Click the **Upload** button in the PlatformIO toolbar.
3. Open the **Serial Monitor** (set to 115200 baud) to track the WiFi and MQTT connection status.

## 📡 Data Format
The application expects MQTT messages in JSON format. It listens for the following topics for each plant (replacing `[ID]` with the specific plant sensor ID):
- `casaortu/sensors/[ID]/temperature`
- `casaortu/sensors/[ID]/soil_moisture`
- `casaortu/sensors/[ID]/luminosity`
- `casaortu/sensors/[ID]/battery_level`

**Example Payload:**
```json
{"value": 22.5}
```

## 🎨 UI Layout
- **Top Row**: Tabs for each plant. Touching a tab switches the view.
- **Main Area**: Displays the current plant's Name, Temperature (°C), Soil Moisture (%), Luminosity (lumen), and Battery level (%).
# cyd_plants
