#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <math.h>

// ===== WIFI =====
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";

#define PIN_LDR   36
#define PIN_NTC   39

#define LED_TEMP_RED    12
#define LED_TEMP_GREEN  13
#define LED_LDR_RED     2
#define LED_LDR_GREEN   15

// ===== ADC =====
static const float VREF = 3.3f;
static const int   ADC_MAX = 4095;
  
// ===== PONTS DIVISEURS =====
static const float R_FIXED_LDR = 1000.0f;
static const float R_FIXED_NTC = 47000.0f;

// ===== NTC =====
static const float R_25     = 10000.0f;
static const float B_25_100 = 3988.0f;
static const float T_25_K   = 298.15f;


float TEMP_MIN = 10.0f;
float TEMP_MAX = 30.0f;
float rLdr_MIN = 20.0f;

bool manualMode = false;


WebServer server(80);
TFT_eSPI tft = TFT_eSPI();


float adcToVoltage(int raw) {
  return (raw * VREF) / ADC_MAX;
}

float ntcResistanceFromVout(float vout) {
  if (vout <= 0.01f || vout >= (VREF - 0.01f)) return NAN;
  return R_FIXED_NTC * ((VREF - vout) / vout);
}

float ntcToCelsius(float vout) {
  float rNtc = ntcResistanceFromVout(vout);
  if (isnan(rNtc) || rNtc <= 0.0f) return NAN;

  float invT = (1.0f / T_25_K) +
               (1.0f / B_25_100) * log(rNtc / R_25);
  return (1.0f / invT) - 273.15f;
}

float ldrResistanceFromVout(float vout) {
  if (vout <= 0.001f || vout >= (VREF - 0.001f)) return NAN;
  return R_FIXED_LDR * (vout / (VREF - vout));
}

void handleSensors() {
  server.send(200, "application/json",
              "{\"sensors\":[\"ldr\",\"thermistor\"]}");
}

void handleLdr() {
  int raw = analogRead(PIN_LDR);
  float v = adcToVoltage(raw);
  float r = ldrResistanceFromVout(v);

  StaticJsonDocument<128> doc;
  doc["raw"] = raw;
  doc["voltage"] = v;
  if (!isnan(r)) doc["res_ohm"] = r;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleTemp() {
  int raw = analogRead(PIN_NTC);
  float v = adcToVoltage(raw);
  float c = ntcToCelsius(v);

  StaticJsonDocument<128> doc;
  doc["raw"] = raw;
  doc["voltage"] = v;
  if (!isnan(c)) doc["celsius"] = c;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleTempConfig() {
  if (server.hasArg("min")) TEMP_MIN = server.arg("min").toFloat();
  if (server.hasArg("max")) TEMP_MAX = server.arg("max").toFloat();

  StaticJsonDocument<128> doc;
  doc["TEMP_MIN"] = TEMP_MIN;
  doc["TEMP_MAX"] = TEMP_MAX;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleLDRConfig() {
  if (server.hasArg("min")) rLdr_MIN = server.arg("min").toFloat();

  StaticJsonDocument<128> doc;
  doc["rLdr_MIN"] = rLdr_MIN;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleLedManual() {
  if (!server.hasArg("id") || !server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"missing args\"}");
    return;
  }

  manualMode = true;

  String id = server.arg("id");
  String state = server.arg("state");

  auto apply = [&](int pin) {
    if (state == "on") digitalWrite(pin, HIGH);
    else if (state == "off") digitalWrite(pin, LOW);
    else if (state == "toggle") digitalWrite(pin, !digitalRead(pin));
  };

  if (id == "1R") apply(LED_TEMP_RED);
  else if (id == "1V") apply(LED_TEMP_GREEN);
  else if (id == "2R") apply(LED_LDR_RED);
  else if (id == "2V") apply(LED_LDR_GREEN);
  else if (id == "all") {
    apply(LED_TEMP_RED);
    apply(LED_TEMP_GREEN);
    apply(LED_LDR_RED);
    apply(LED_LDR_GREEN);
  } else {
    server.send(400, "application/json", "{\"error\":\"unknown led\"}");
    return;
  }

  server.send(200, "application/json", "{\"mode\":\"manual\"}");
}

void handleMode() {
  if (server.hasArg("set")) {
    manualMode = (server.arg("set") == "manual");
  }

  StaticJsonDocument<64> doc;
  doc["mode"] = manualMode ? "manual" : "auto";

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(LED_TEMP_RED, OUTPUT);
  pinMode(LED_TEMP_GREEN, OUTPUT);
  pinMode(LED_LDR_RED, OUTPUT);
  pinMode(LED_LDR_GREEN, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_LDR, ADC_11db);
  analogSetPinAttenuation(PIN_NTC, ADC_11db);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());


  server.on("/sensors", handleSensors);
  server.on("/ldr", handleLdr);
  server.on("/temp", handleTemp);
  server.on("/config/temp", handleTempConfig);
  server.on("/config/ldr", handleLDRConfig);
  server.on("/led", handleLedManual);   
  server.on("/mode", handleMode);      

  server.begin();
}


void loop() {
  server.handleClient();

  float tempC = ntcToCelsius(adcToVoltage(analogRead(PIN_NTC)));
  float rLdr  = ldrResistanceFromVout(adcToVoltage(analogRead(PIN_LDR)));
  
--
  if (!manualMode) {
    if (!isnan(tempC) && (tempC < TEMP_MIN || tempC > TEMP_MAX)) {
      digitalWrite(LED_TEMP_RED, HIGH);
      digitalWrite(LED_TEMP_GREEN, LOW);
    } else {
      digitalWrite(LED_TEMP_RED, LOW);
      digitalWrite(LED_TEMP_GREEN, HIGH);
    }

    if (isnan(rLdr) || rLdr < rLdr_MIN) {
      digitalWrite(LED_LDR_RED, HIGH);
      digitalWrite(LED_LDR_GREEN, LOW);
    } else {
      digitalWrite(LED_LDR_RED, LOW);
      digitalWrite(LED_LDR_GREEN, HIGH);
    }
  }

  tft.setCursor(0, 0);
  tft.fillRect(0, 0, 240, 135, TFT_BLACK);
  tft.printf("Mode: %s\n", manualMode ? "MANUEL" : "AUTO");
  tft.printf("Temp: %.2f C\n", tempC);
  tft.printf("LDR: %.0f ohm\n", rLdr);

  delay(400);
}
