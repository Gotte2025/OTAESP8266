/*
  OTA por Internet para ESP8266
  ------------------------------
  El equipo consulta un archivo version.json cada X minutos.
  Si la versión remota es distinta a la actual, descarga y
  aplica el nuevo firmware.bin automáticamente, y reinicia.

  Requiere la librería "ArduinoJson" (Sketch > Include Library > Manage Libraries).
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

// ---------- CONFIGURACIÓN ----------
const char* ssid     = "TU_WIFI";
const char* password = "TU_PASSWORD";

// URL pública del archivo version.json (raw de GitHub, Vercel, etc.)
const char* versionURL = "https://raw.githubusercontent.com/tu_usuario/tu_repo/main/version.json";

// Versión que tiene ESTE firmware compilado. Subila en cada release.
const String FIRMWARE_VERSION = "1.0.0";

// Intervalo de chequeo (10 minutos)
const unsigned long CHECK_INTERVAL = 10UL * 60UL * 1000UL;
// ------------------------------------

unsigned long lastCheck = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
  Serial.println("Versión actual: " + FIRMWARE_VERSION);

  checkForUpdate(); // chequeo apenas arranca
}

void loop() {
  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForUpdate();
  }

  // ---- Acá va el resto de la lógica normal de tu equipo ----
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("\n[OTA] Consultando version.json...");

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // ver nota de seguridad en el README

  HTTPClient https;
  if (!https.begin(*client, versionURL)) {
    Serial.println("[OTA] No se pudo iniciar la conexión");
    return;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] Error HTTP al pedir version.json: %d\n", httpCode);
    https.end();
    return;
  }

  String payload = https.getString();
  https.end();

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[OTA] Error parseando JSON: ");
    Serial.println(err.c_str());
    return;
  }

  String remoteVersion = doc["version"] | "";
  String firmwareURL   = doc["url"] | "";

  Serial.println("[OTA] Versión remota: " + remoteVersion);

  if (remoteVersion.length() == 0 || firmwareURL.length() == 0) {
    Serial.println("[OTA] version.json inválido");
    return;
  }

  if (remoteVersion == FIRMWARE_VERSION) {
    Serial.println("[OTA] Ya estás en la última versión.");
    return;
  }

  Serial.println("[OTA] Nueva versión disponible -> actualizando...");
  performUpdate(firmwareURL);
}

void performUpdate(const String& firmwareURL) {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  ESPhttpUpdate.onStart([]() { Serial.println("[OTA] Descarga iniciada..."); });
  ESPhttpUpdate.onEnd([]()   { Serial.println("[OTA] Descarga completa."); });
  ESPhttpUpdate.onProgress([](int cur, int total) {
    Serial.printf("[OTA] Progreso: %d%%\n", (cur * 100) / total);
  });
  ESPhttpUpdate.onError([](int error) {
    Serial.printf("[OTA] Error: %d\n", error);
  });

  t_httpUpdate_return ret = ESPhttpUpdate.update(*client, firmwareURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] Falló (%d): %s\n",
                     ESPhttpUpdate.getLastError(),
                     ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] El servidor dijo que no hay updates.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] Listo. Reiniciando...");
      // El ESP se reinicia solo tras un update exitoso.
      break;
  }
}
