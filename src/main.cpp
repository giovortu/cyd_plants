#include <WiFi.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <vector>
#include "config.h"

#define SD_CS 5


struct Plant {
    String name;
    String id;
    float temp;
    float moisture;
    float light;
    float battery;
    String topic_base;

    Plant(String n, String i, String tb) 
        : name(n), id(i), temp(0), moisture(0), light(0), battery(0), topic_base(tb) {}
};

struct Settings {
    String ssid;
    String password;
    String mqtt_server;
    int mqtt_port;
    String mqtt_user;
    String mqtt_pass;
} appSettings;

std::vector<Plant> plants;
int currentTab = 0;
int firstVisiblePlantIdx = 0;
const int MAX_VISIBLE_PLANTS = 3;

TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient client(espClient);

// --- Settings Logic ---

void saveSettings() {
    File file = SD.open("/settings.json", FILE_WRITE);
    if (file) {
        StaticJsonDocument<512> doc;
        doc["ssid"] = appSettings.ssid;
        doc["password"] = appSettings.password;
        doc["mqtt_server"] = appSettings.mqtt_server;
        doc["mqtt_port"] = appSettings.mqtt_port;
        doc["mqtt_user"] = appSettings.mqtt_user;
        doc["mqtt_pass"] = appSettings.mqtt_pass;
        serializeJson(doc, file);
        file.close();
    }
}

bool loadSettings() {
    File file = SD.open("/settings.json");
    if (file) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (!error) {
            appSettings.ssid = doc["ssid"] | ssid;
            appSettings.password = doc["password"] | password;
            appSettings.mqtt_server = doc["mqtt_server"] | mqtt_server;
            appSettings.mqtt_port = doc["mqtt_port"] | mqtt_port;
            appSettings.mqtt_user = doc["mqtt_user"] | mqtt_user;
            appSettings.mqtt_pass = doc["mqtt_pass"] | mqtt_pass;
            return true;
        }
    }
    appSettings.ssid = ssid;
    appSettings.password = password;
    appSettings.mqtt_server = mqtt_server;
    appSettings.mqtt_port = mqtt_port;
    appSettings.mqtt_user = mqtt_user;
    appSettings.mqtt_pass = mqtt_pass;
    return false;
}

// --- UI Helper Components ---

void drawGauge(int y, float value, float minVal, float maxVal, String label, String unit, uint16_t color = TFT_BLUE) {
    int labelWidth = 110;
    int barX = labelWidth + 10;
    int barW = tft.width() - barX - 30;
    int barH = 25;

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TR_DATUM);
    String displayLabel = label;
    if (displayLabel.length() > 15) {
        displayLabel = displayLabel.substring(0, 12) + "...";
    }
    tft.drawString(displayLabel, labelWidth, y + barH/2, 2);

    tft.drawRect(barX, y, barW, barH, TFT_WHITE);

    float percent = constrain((value - minVal) / (maxVal - minVal), 0.0, 1.0);
    int fillW = barW * percent;
    tft.fillRect(barX + 1, y + 1, fillW - 1, barH - 2, color);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    String valStr = String(value, 1) + " " + unit;
    tft.setTextSize(1); 
    tft.drawString(valStr, barX + barW/2, y + barH/2, 2);
}

// --- Application Logic ---

bool loadPlantsFromJson() {
    if (!SD.begin(SD_CS)) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("SD Card Error!", tft.width()/2, tft.height()/2, 4);
        return false;
    }

    File file = SD.open("/plants.json");
    if (!file) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("plants.json missing!", tft.width()/2, tft.height()/2, 2);
        return false;
    }

    DynamicJsonDocument doc(4096);
    deserializeJson(doc, file);
    file.close();

    plants.clear();
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        plants.push_back(Plant(
            obj["name"].as<String>(),
            obj["id"].as<String>(),
            obj["topic_base"].as<String>()
        ));
    }
    return true;
}

void setup_wifi() {
    delay(10);
    WiFi.begin(appSettings.ssid.c_str(), appSettings.password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload, length);
    float value = doc["value"];

    String topicStr = String((char*)topic);
    for (size_t i = 0; i < plants.size(); i++) {
        if (topicStr.startsWith(plants[i].topic_base)) {
            if (topicStr.endsWith("temperature")) plants[i].temp = value;
            else if (topicStr.endsWith("soil_moisture")) plants[i].moisture = value;
            else if (topicStr.endsWith("luminosity")) plants[i].light = value;
            else if (topicStr.endsWith("battery_level")) plants[i].battery = value;
            break;
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("ESP32_CYD_Plants", appSettings.mqtt_user.c_str(), appSettings.mqtt_pass.c_str())) {
            for (size_t i = 0; i < plants.size(); i++) {
                client.subscribe((plants[i].topic_base + "temperature").c_str());
                client.subscribe((plants[i].topic_base + "soil_moisture").c_str());
                client.subscribe((plants[i].topic_base + "luminosity").c_str());
                client.subscribe((plants[i].topic_base + "battery_level").c_str());
            }
        } else {
            delay(5000);
        }
    }
}

void drawTabs() {
    tft.fillScreen(TFT_BLACK);
    if (plants.empty()) return;

    int arrowWidth = 30;
    int settingsWidth = 60;
    int availableWidth = tft.width() - (2 * arrowWidth) - settingsWidth;
    int plantTabWidth = availableWidth / MAX_VISIBLE_PLANTS;

    // Draw Left Arrow (Vertex at x=5, Base at x=25)
    tft.fillTriangle(5, 15, arrowWidth - 5, 8, arrowWidth - 5, 22, TFT_WHITE);

    for (int i = 0; i < MAX_VISIBLE_PLANTS; i++) {
        int plantIdx = firstVisiblePlantIdx + i;
        int xPos = arrowWidth + (i * plantTabWidth);
        
        if (plantIdx < (int)plants.size()) {
            if (currentTab == plantIdx) tft.fillRoundRect(xPos, 0, plantTabWidth - 2, 30, 4, TFT_DARKGREY);
            else tft.fillRoundRect(xPos, 0, plantTabWidth - 2, 30, 4, TFT_BLACK);
            
            tft.setTextColor(currentTab == plantIdx ? TFT_WHITE : TFT_LIGHTGREY);
            tft.setTextDatum(MC_DATUM);
            String name = plants[plantIdx].name;
            int maxChars = (plantTabWidth / 7) - 2; 
            if (name.length() > maxChars && maxChars > 3) {
                name = name.substring(0, maxChars - 3) + "...";
            }
            tft.drawString(name, xPos + plantTabWidth / 2, 15, 2);
        } else {
            tft.fillRect(xPos, 0, plantTabWidth, 30, TFT_BLACK);
        }
    }

    int settingsX = arrowWidth + (MAX_VISIBLE_PLANTS * plantTabWidth);
    if (currentTab == (int)plants.size()) tft.fillRoundRect(settingsX, 0, settingsWidth - 2, 30, 4, TFT_DARKGREY);
    else tft.fillRoundRect(settingsX, 0, settingsWidth - 2, 30, 4, TFT_BLACK);
    
    tft.setTextColor(currentTab == (int)plants.size() ? TFT_WHITE : TFT_LIGHTGREY);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Set", settingsX + settingsWidth / 2, 15, 2);

    // Draw Right Arrow (Vertex at x=315, Base at x=295)
    tft.fillTriangle(tft.width() - 5, 15, tft.width() - arrowWidth + 5, 8, tft.width() - arrowWidth + 5, 22, TFT_WHITE);
}

void drawPlantData() {
    if (currentTab >= (int)plants.size()) return;
    tft.fillRect(0, 40, tft.width(), tft.height() - 40, TFT_BLACK);
    
    drawGauge(60, plants[currentTab].temp, 0, 50, "Temperature", "C");
    drawGauge(100, plants[currentTab].moisture, 0, 100, "Moisture", "%");
    drawGauge(140, plants[currentTab].light, 0, 5000, "Luminosity", "lx");
    
    uint16_t bColor = TFT_BLUE;
    if (plants[currentTab].battery < 20) bColor = TFT_RED;
    else if (plants[currentTab].battery < 50) bColor = TFT_YELLOW;
    
    drawGauge(180, plants[currentTab].battery, 0, 100, "Battery", "%", bColor);
}

void drawSettingsPage() {
    tft.fillRect(0, 40, tft.width(), tft.height() - 40, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("--- Network Settings ---", 20, 60, 2);
    tft.setTextSize(1);
    tft.drawString("SSID: " + appSettings.ssid, 20, 80, 2);
    tft.drawString("MQTT: " + appSettings.mqtt_server, 20, 100, 2);
    tft.fillRoundRect(20, 140, 140, 40, 4, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Reload Config", 20 + 70, 140 + 20, 2);
    tft.fillRoundRect(170, 140, 140, 40, 4, TFT_RED);
    tft.drawString("Reset Default", 170 + 70, 140 + 20, 2);
}

void handleTouch() {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (y < 30) {
            int arrowWidth = 30;
            int settingsWidth = 60;
            int plantTabWidth = (tft.width() - (2 * arrowWidth) - settingsWidth) / MAX_VISIBLE_PLANTS;

            if (x < arrowWidth && firstVisiblePlantIdx > 0) {
                firstVisiblePlantIdx--;
                drawTabs();
                delay(200);
            } 
            else if (x >= arrowWidth && x < (arrowWidth + MAX_VISIBLE_PLANTS * plantTabWidth)) {
                int localIdx = (x - arrowWidth) / plantTabWidth;
                int plantIdx = firstVisiblePlantIdx + localIdx;
                if (plantIdx < (int)plants.size()) {
                    currentTab = plantIdx;
                    drawTabs();
                    drawPlantData();
                    delay(200);
                }
            } 
            else if (x >= (arrowWidth + MAX_VISIBLE_PLANTS * plantTabWidth) && x < (arrowWidth + MAX_VISIBLE_PLANTS * plantTabWidth + settingsWidth)) {
                currentTab = (int)plants.size();
                drawTabs();
                drawSettingsPage();
                delay(200);
            } 
            else if (x >= (tft.width() - arrowWidth) && (firstVisiblePlantIdx + MAX_VISIBLE_PLANTS < (int)plants.size())) {
                firstVisiblePlantIdx++;
                drawTabs();
                delay(200);
            }
        } else if (currentTab == (int)plants.size()) {
            if (x > 20 && x < 160 && y > 140 && y < 180) {
                loadSettings();
                drawSettingsPage();
            } else if (x > 170 && x < 310 && y > 140 && y < 180) {
                SD.remove("/settings.json");
                loadSettings();
                drawSettingsPage();
            }
        }
    }
}

void setup() {
    Serial.begin(115200);


    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    uint16_t calData[5] = { 275, 3874, 272, 3856, 1 }; 
    tft.setTouch(calData);
    loadSettings();
    if (!loadPlantsFromJson()) {
        while(1) { delay(1000); }
    }
    setup_wifi();
    client.setServer(appSettings.mqtt_server.c_str(), appSettings.mqtt_port);
    client.setCallback(mqtt_callback);
    drawTabs();
    drawPlantData();

        
Serial.print("Width: "); Serial.println(tft.width());
Serial.print("Height: "); Serial.println(tft.height());
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();
    handleTouch();
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {
        if (currentTab < (int)plants.size()) drawPlantData();
        lastUpdate = millis();
    }
}
