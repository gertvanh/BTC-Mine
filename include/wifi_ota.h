#pragma once

#include <Arduino.h>

void wifiOtaBegin();
void wifiOtaLoop();
bool wifiOtaConnected();
bool wifiOtaPortalActive();
String wifiOtaIpString();
String wifiOtaSsid();
String wifiOtaStatusLine();
void wifiOtaCheckForUpdate(bool force = false);
void wifiOtaResetAndReboot();
