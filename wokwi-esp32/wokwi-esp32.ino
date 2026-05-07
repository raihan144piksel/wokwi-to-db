#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- KONFIGURASI ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "";
const char* mqtt_pass   = "";

// --- NTP SERVER CONFIG ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // Keep it at 0 for UTC time (Best practice for databases)
const int   daylightOffset_sec = 0;

#define DHTPIN 15
#define DHTTYPE DHT22
#define LDR_PIN 34
#define LED_TEMP 2
#define LED_HUM 4
#define LED_LIGHT 5

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0; // Timer variable for non-blocking delay
unsigned long lastReconnectAttempt = 0; // Timer for non-blocking MQTT reconnect
String deviceId; // Variable to hold the unique MAC address

void setup_wifi() {
  Serial.print("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Set Device ID to MAC Address
  deviceId = WiFi.macAddress();
  deviceId.replace(":", ""); // Remove colons to make it cleaner (e.g., 246F28A2C1D0)
  Serial.println("Device ID: " + deviceId);
}

void setup_time() {
  Serial.print("Syncing time via NTP...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait until time is synced
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTime synced successfully!");
}

boolean reconnect() {
  Serial.print("Attempting MQTT connection...");
  
  // Use the unique Device ID for the MQTT Client ID
  String clientId = "ESP32-" + deviceId;
  
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println("connected!");
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_HUM, OUTPUT);
  pinMode(LED_LIGHT, OUTPUT);
  dht.begin();
  setup_wifi();
  setup_time();
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  lastReconnectAttempt = 0;
}

void loop() {
  client.loop();
  
  unsigned long now = millis();
  
  if (!client.connected()) {
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop(); // Only loop if connected
  }

  // Non-blocking 5-second timer
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      Serial.println("Gagal membaca sensor DHT!");
      return; // Skip cycle ini
    }

    int ldrValue = analogRead(LDR_PIN);
    if (ldrValue >= 4095) ldrValue = 4094;
    float light = (ldrValue / 4095.0) * 100;
    float voltage = ldrValue / 4095.0 * 3.3;
    float resistance = 2000 * voltage / (1 - voltage / 3.3);
    float lux = pow(50 * 1e3 * pow(10, 0.7) / resistance, (1 / 0.7));

    // 2. Logika Lokal (Aktuator)
    digitalWrite(LED_TEMP, t > 30 ? HIGH : LOW);
    digitalWrite(LED_HUM, h < 40 ? HIGH : LOW);
    digitalWrite(LED_LIGHT, light < 50 ? HIGH : LOW);

    time_t now_time;
    time(&now_time);

    // 3. Membuat JSON Payload
    // StaticJsonDocument<200> mengalokasikan memori di stack
    JsonDocument doc;
    doc["device_id"] = deviceId;      // Added Device ID
    doc["timestamp"] = now_time;      // Added UTC Unix Timestamp
    doc["temperature"] = t;
    doc["humidity"] = h;
    doc["light_level"] = light;
    doc["status_led_temp"] = (t > 30); // true/false
    doc["light_lux"] = lux;

    char buffer[256];
    serializeJson(doc, buffer);
    
    if (client.connected()) {
      client.publish("wokwi/esp32/telemetry", buffer);
      Serial.print("Published JSON: ");
      Serial.println(buffer);
    } else {
      Serial.println("Skipped publish: MQTT not connected");
    }
  }
}