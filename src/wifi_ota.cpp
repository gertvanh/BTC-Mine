#include "wifi_ota.h"

#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

#include "ota_github.h"
#include "secrets.h"
#include "version.h"

namespace {

WiFiManager wm;
unsigned long lastHttpCheckMs = 0;
bool loggedConnection = false;
bool otaReady = false;
bool portalActive = false;
bool bootstrapped = false;

constexpr unsigned long kHttpCheckMs = 60UL * 60UL * 1000UL;
constexpr unsigned long kDefaultWifiTimeoutMs = 15000;
constexpr int kBootButtonPin = 0;  // ESP32 BOOT button
constexpr char kSetupApName[] = "BTC-Mine-setup";

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

bool tryDefaultNetwork() {
  Serial.printf("[WiFi] trying default SSID '%s' (DHCP)...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(FW_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < kDefaultWifiTimeoutMs) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] default network connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("[WiFi] default network failed");
  WiFi.disconnect(true, true);
  delay(100);
  return false;
}

void startConfigPortal() {
  portalActive = true;
  Serial.printf("[WiFi] starting setup AP '%s'\n", kSetupApName);
  Serial.println("[WiFi] connect phone to that AP, open http://192.168.4.1");
  wm.startConfigPortal(kSetupApName);
}

void onApCallback(WiFiManager* /*wifiManager*/) {
  portalActive = true;
  Serial.printf("[WiFi] config portal active — join WiFi '%s'\n", kSetupApName);
}

bool bootButtonHeld() {
  pinMode(kBootButtonPin, INPUT_PULLUP);
  delay(20);
  if (digitalRead(kBootButtonPin) != LOW) return false;
  // Require ~1.5s hold to avoid accidental resets
  const unsigned long start = millis();
  while (digitalRead(kBootButtonPin) == LOW) {
    if (millis() - start >= 1500) return true;
    delay(20);
  }
  return false;
}

void handleSerialCommands() {
  static String line;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      line.trim();
      if (line.equalsIgnoreCase("wifi-reset")) {
        wifiOtaResetAndReboot();
      } else if (line.equalsIgnoreCase("ota-check")) {
        wifiOtaCheckForUpdate(true);
      } else if (line.length() > 0) {
        Serial.println("[CMD] unknown (try: wifi-reset | ota-check)");
      }
      line = "";
    } else if (line.length() < 64) {
      line += c;
    }
  }
}

}  // namespace

void wifiOtaResetAndReboot() {
  Serial.println("[WiFi] clearing saved credentials and rebooting...");
  wm.resetSettings();
  WiFi.disconnect(true, true);
  delay(300);
  ESP.restart();
}

void wifiOtaBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(FW_HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  wm.setDebugOutput(false);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);  // stay open until configured
  wm.setConnectTimeout(25);
  wm.setTitle("BTC-Mine WiFi");
  wm.setHostname(FW_HOSTNAME);
  wm.setAPCallback(onApCallback);

  if (bootButtonHeld()) {
    Serial.println("[WiFi] BOOT held — wipe WiFi settings");
    wm.resetSettings();
    WiFi.disconnect(true, true);
    delay(200);
  }

  // Prefer credentials saved via the captive portal.
  if (wm.getWiFiIsSaved()) {
    Serial.println("[WiFi] connecting with saved credentials (DHCP)...");
    portalActive = false;
    wm.autoConnect(kSetupApName);
  } else if (tryDefaultNetwork()) {
    portalActive = false;
  } else {
    startConfigPortal();
  }

  bootstrapped = true;
  lastHttpCheckMs = 0;
}

bool wifiOtaConnected() { return WiFi.status() == WL_CONNECTED; }

bool wifiOtaPortalActive() {
  return portalActive || wm.getConfigPortalActive();
}

String wifiOtaIpString() {
  if (!wifiOtaConnected()) return String("--");
  return WiFi.localIP().toString();
}

String wifiOtaSsid() {
  if (!wifiOtaConnected()) return String("");
  return WiFi.SSID();
}

String wifiOtaStatusLine() {
  if (wifiOtaConnected()) return wifiOtaIpString();
  if (wifiOtaPortalActive()) return String(kSetupApName);
  return String("WiFi...");
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
  if (!bootstrapped) return;

  handleSerialCommands();
  wm.process();

  const bool connected = wifiOtaConnected();
  portalActive = wm.getConfigPortalActive();

  if (connected) {
    if (portalActive) {
      // Connected through portal completion — close AP side effects.
      portalActive = false;
    }
    if (!loggedConnection) {
      loggedConnection = true;
      Serial.print("[WiFi] connected to ");
      Serial.print(WiFi.SSID());
      Serial.print(", IP: ");
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

  // If we dropped offline and no portal is running, reopen setup after a while.
  static unsigned long lastPortalRetryMs = 0;
  const unsigned long now = millis();
  if (!wifiOtaPortalActive() && (now - lastPortalRetryMs) > 30000) {
    lastPortalRetryMs = now;
    if (wm.getWiFiIsSaved()) {
      Serial.println("[WiFi] retry saved network...");
      wm.autoConnect(kSetupApName);
    } else {
      startConfigPortal();
    }
  }
}
