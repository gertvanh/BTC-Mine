#pragma once

#include <Arduino.h>

void wifiOtaBegin();
void wifiOtaLoop();
bool wifiOtaConnected();
String wifiOtaIpString();
void wifiOtaCheckForUpdate(bool force = false);
