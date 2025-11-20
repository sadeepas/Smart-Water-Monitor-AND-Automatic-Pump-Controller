/*
 * ESP32 Water Level Controller - Full Featured Standalone Server
 *
 * This firmware creates a Wi-Fi Access Point and serves a complete web application
 * from its internal SPIFFS file system.
 *
 * Features:
 * - Creates its own Wi-Fi network (Access Point Mode). No router needed.
 * - Serves 'index.html' from the SPIFFS file system.
 * - Provides real-time data to all connected clients via WebSockets.
 * - Receives configuration changes (threshold, tank dimensions) via WebSockets.
 * - Full safety logic: Hysteresis, top overflow sensor, dry-run detection, max pump runtime.
 */

// Required Libraries (Install via Arduino Library Manager)
#include <WiFi.h>              // For Wi-Fi functionality
#include <ESPAsyncWebServer.h> // For the web and WebSocket server
#include <ArduinoJson.h>       // For WebSocket communication
#include "SPIFFS.h"            // For serving files from flash memory

// --- PIN DEFINITIONS (Verify with your hardware) ---
const int TRIGGER_PIN = 5;
const int ECHO_PIN = 18;
const int TOP_SENSOR_PIN = 4;
const int PUMP_PIN = 2;

// --- ACCESS POINT CREDENTIALS (This is the Wi-Fi network the ESP32 creates) ---
const char* ap_ssid = "WaterMonitorAP";
const char* ap_password = "pump12345"; // Password must be 8+ characters

// --- TUNABLE SAFETY & PERFORMANCE PARAMETERS ---
const int HYSTERESIS_PERCENT = 10;
const unsigned long PUMP_MAX_RUNTIME_MS = 300000UL; // 5 minutes
const unsigned long MIN_PUMP_RUN_MS = 30000UL;      // 30 seconds
const unsigned long DATA_BROADCAST_INTERVAL_MS = 1000UL; // Send data every second

// --- TANK CONFIGURATION (Defaults, can be updated from web UI) ---
float tank_height = 100.0f;
float tank_radius = 30.0f;
int low_threshold_percent = 20;

// --- GLOBAL STATE VARIABLES ---
int waterPercentage = 0;
int lastWaterPercentage = 0;
bool pumpState = false;
unsigned long pumpStartTs = 0;
int high_threshold_percent = low_threshold_percent + HYSTERESIS_PERCENT;
unsigned long lastBroadcastTime = 0;

// --- SERVER & WEBSOCKET SETUP ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket endpoint: ws://192.168.4.1/ws

// Function Prototypes
void broadcastData();
void setPump(bool on);

// Handles incoming WebSocket events (connections, disconnections, messages)
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    // Send the latest data to the newly connected client immediately
    broadcastData();
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket Client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    // Data received from a client (e.g., config updates)
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      data[len] = 0; // Null-terminate the received data
      Serial.printf("Received WebSocket data: %s\n", (char*)data);
      
      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
        if (doc.containsKey("threshold")) {
          low_threshold_percent = doc["threshold"].as<int>();
          high_threshold_percent = constrain(low_threshold_percent + HYSTERESIS_PERCENT, 0, 100);
          Serial.printf("Threshold updated to %d%%\n", low_threshold_percent);
        }
        if (doc.containsKey("height")) tank_height = doc["height"].as<float>();
        if (doc.containsKey("radius")) tank_radius = doc["radius"].as<float>();
        // After any config change, broadcast the new state to all clients
        broadcastData();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Full-Featured Water Monitor");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Pin setup
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TOP_SENSOR_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  // Start the Wi-Fi Access Point
  Serial.printf("Creating Access Point: %s\n", ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP()); // Default is 192.168.4.1

  // Attach WebSocket handler
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve the main web page from SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Handle page not found
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Server started. Connect to the AP and navigate to http://192.168.4.1");
}

void loop() {
  // All client handling is done asynchronously in the background.
  // The main loop is only for our core sensor and pump logic.
  
  if (millis() - lastBroadcastTime > DATA_BROADCAST_INTERVAL_MS) {
    // --- 1. SENSOR READING & STATE CALCULATION ---
    float waterHeight_cm = max(0.0f, tank_height - getDistance());
    waterPercentage = constrain((int)((waterHeight_cm / tank_height) * 100.0f), 0, 100);

    // --- 2. PUMP LOGIC & SAFETY CHECKS ---
    bool wantPump;
    if (waterPercentage < low_threshold_percent) wantPump = true;
    else if (waterPercentage > high_threshold_percent) wantPump = false;
    else wantPump = pumpState; // Hysteresis: Maintain state in the dead-band

    if (digitalRead(TOP_SENSOR_PIN) == LOW) wantPump = false; // Safety: Top sensor triggered
    if (pumpState) { // Additional safety checks only if pump is already on
        unsigned long runtime = millis() - pumpStartTs;
        if (runtime > PUMP_MAX_RUNTIME_MS) wantPump = false; // Safety: Max runtime exceeded
        if (runtime > MIN_PUMP_RUN_MS && waterPercentage <= lastWaterPercentage) wantPump = false; // Safety: Dry run detected
    }

    setPump(wantPump);
    lastWaterPercentage = waterPercentage;

    // --- 3. BROADCAST DATA TO ALL CONNECTED CLIENTS ---
    broadcastData();
    lastBroadcastTime = millis();
  }
}

// --- Helper Functions ---

float getDistance() {
  digitalWrite(TRIGGER_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  return (duration > 0) ? (duration * 0.0343) / 2.0f : tank_height + 10; // Return out of range on timeout
}

void setPump(bool on) {
  if (on != pumpState) {
    pumpState = on;
    digitalWrite(PUMP_PIN, pumpState ? HIGH : LOW);
    if (pumpState) pumpStartTs = millis();
    Serial.printf("Pump turned %s\n", pumpState ? "ON" : "OFF");
  }
}

void broadcastData() {
    float waterHeight_cm = max(0.0f, tank_height - getDistance());
    float volume_l = (PI * tank_radius * tank_radius * waterHeight_cm) / 1000.0f;
    int currentPercentage = constrain((int)((waterHeight_cm / tank_height) * 100.0f), 0, 100);
    
    DynamicJsonDocument doc(512);
    doc["water"] = currentPercentage;
    doc["pump"] = pumpState ? 1 : 0;
    doc["threshold"] = low_threshold_percent;
    doc["topTriggered"] = (digitalRead(TOP_SENSOR_PIN) == LOW) ? 1 : 0;
    doc["height"] = (int)waterHeight_cm;
    doc["volume"] = round(volume_l * 10) / 10.0f;
    doc["tankHeight"] = (int)tank_height;
    doc["tankRadius"] = (int)tank_radius;
    
    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);
}