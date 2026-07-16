#include "wifi_ota.h"

#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "ota_github.h"
#include "secrets.h"
#include "version.h"

namespace {

unsigned long lastWifiAttemptMs = 0;
unsigned long lastHttpCheckMs = 0;
bool loggedConnection = false;
bool otaReady = false;

constexpr unsigned long kWifiRetryMs = 10000;
constexpr unsigned long kHttpCheckMs = 60UL * 60UL * 1000UL;  // 1 hour

String otaVersionUrl() {
#ifdef OTA_VERSION_URL
  if (strlen(OTA_VERSION_URL) > 0) {
    return String(OTA_VERSION_URL);
  }
#endif
  return String("https://github.com/") + OTA_GITHUB_OWNER + "/" + OTA_GITHUB_REPO +
         "/releases/latest/download/version.json";
}

void configureHttpClient(HTTPClient& http) {
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent(String("BTC-Mine-ESP32/") + FW_VERSION);
  http.setReuse(false);
}

int versionCompare(const char* a, const char* b) {
  int a1 = 0, a2 = 0, a3 = 0;
  int b1 = 0, b2 = 0, b3 = 0;
  sscanf(a, "%d.%d.%d", &a1, &a2, &a3);
  sscanf(b, "%d.%d.%d", &b1, &b2, &b3);
  if (a1 != b1) return a1 < b1 ? -1 : 1;
  if (a2 != b2) return a2 < b2 ? -1 : 1;
  if (a3 != b3) return a3 < b3 ? -1 : 1;
  return 0;
}

bool extractJsonString(const String& json, const char* key, String& out) {
  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return false;
  int colon = json.indexOf(':', keyPos + pattern.length());
  if (colon < 0) return false;
  int firstQuote = json.indexOf('"', colon + 1);
  if (firstQuote < 0) return false;
  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return false;
  out = json.substring(firstQuote + 1, secondQuote);
  return out.length() > 0;
}

void setupArduinoOta() {
  ArduinoOTA.setHostname(FW_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  ArduinoOTA.onStart([]() { Serial.println("[OTA] LAN update start"); });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] LAN update done, reboot..."); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] LAN error %u\n", e); });
  ArduinoOTA.begin();
  otaReady = true;
  Serial.printf("[OTA] ArduinoOTA ready as '%s'\n", FW_HOSTNAME);
}

}  // namespace

void wifiOtaBegin() {
  WiFi.mode(WIFI_STA);

  const IPAddress localIP(WIFI_IP_1, WIFI_IP_2, WIFI_IP_3, WIFI_IP_4);
  const IPAddress gateway(WIFI_GATEWAY_1, WIFI_GATEWAY_2, WIFI_GATEWAY_3, WIFI_GATEWAY_4);
  const IPAddress subnet(WIFI_SUBNET_1, WIFI_SUBNET_2, WIFI_SUBNET_3, WIFI_SUBNET_4);
  const IPAddress dns(WIFI_DNS_1, WIFI_DNS_2, WIFI_DNS_3, WIFI_DNS_4);

  if (!WiFi.config(localIP, gateway, subnet, dns)) {
    Serial.println("[WiFi] static IP config failed");
  }

  Serial.printf("[WiFi] connecting to %s as %d.%d.%d.%d\n", WIFI_SSID, WIFI_IP_1, WIFI_IP_2,
                WIFI_IP_3, WIFI_IP_4);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptMs = millis();
}

bool wifiOtaConnected() { return WiFi.status() == WL_CONNECTED; }

String wifiOtaIpString() {
  if (!wifiOtaConnected()) return String("—.——.—.—");
  return WiFi.localIP().toString();
}

void wifiOtaCheckForUpdate(bool force) {
  if (!wifiOtaConnected()) return;

  const unsigned long now = millis();
  if (!force && lastHttpCheckMs != 0 && (now - lastHttpCheckMs) < kHttpCheckMs) {
    return;
  }
  lastHttpCheckMs = now;

  const String versionUrl = otaVersionUrl();
  Serial.printf("[OTA] checking %s (running %s)\n", versionUrl.c_str(), FW_VERSION);

  HTTPClient http;
  WiFiClientSecure secureClient;
  const bool versionHttps = versionUrl.startsWith("https://");
  bool began = false;
  if (versionHttps) {
    secureClient.setInsecure();
    began = http.begin(secureClient, versionUrl);
  } else {
    began = http.begin(versionUrl);
  }
  if (!began) {
    Serial.println("[OTA] version URL begin failed");
    return;
  }
  configureHttpClient(http);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] version GET failed: %d\n", code);
    http.end();
    return;
  }

  const String body = http.getString();
  http.end();

  String remoteVersion;
  String firmwareUrl;
  if (!extractJsonString(body, "version", remoteVersion) ||
      !extractJsonString(body, "url", firmwareUrl)) {
    Serial.println("[OTA] invalid version.json");
    return;
  }

  Serial.printf("[OTA] remote=%s url=%s\n", remoteVersion.c_str(), firmwareUrl.c_str());
  if (versionCompare(remoteVersion.c_str(), FW_VERSION) <= 0) {
    Serial.println("[OTA] already up to date");
    return;
  }

  Serial.println("[OTA] starting HTTP(S) update...");
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = HTTP_UPDATE_FAILED;
  if (firmwareUrl.startsWith("https://")) {
    WiFiClientSecure fwClient;
    fwClient.setInsecure();
    ret = httpUpdate.update(fwClient, firmwareUrl);
  } else {
    WiFiClient fwClient;
    ret = httpUpdate.update(fwClient, firmwareUrl);
  }
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] HTTP update failed (%d): %s\n", httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] no updates");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] update ok");
      break;
  }
}

void wifiOtaLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!loggedConnection) {
      loggedConnection = true;
      Serial.print("[WiFi] connected, IP: ");
      Serial.println(WiFi.localIP());
      if (!otaReady) {
        setupArduinoOta();
        wifiOtaCheckForUpdate(true);
      }
    }
    if (otaReady) {
      ArduinoOTA.handle();
    }
    wifiOtaCheckForUpdate(false);
    return;
  }

  if (loggedConnection) {
    loggedConnection = false;
    otaReady = false;
    Serial.println("[WiFi] disconnected");
  }

  const unsigned long now = millis();
  if (now - lastWifiAttemptMs < kWifiRetryMs) return;
  lastWifiAttemptMs = now;
  Serial.println("[WiFi] reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
