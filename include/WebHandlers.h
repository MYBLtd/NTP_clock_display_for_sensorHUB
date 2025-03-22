// WebSHandlers.h
#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "WebContent.h"
#include "RelayControlHandler.h"
#include "WebServerManager.h"

// Declare external WebServer instance
extern WebServer server;

// Handler function declarations
void handleRoot();
void handleScan();
void handleConnect();
void handleGetPreferences();
void handleSetPreferences();
void handleOptionsPreferences();
void handleCaptivePortal();
void handleIcon();
void handleGetRelayState();
void handleSetRelayState();
void handleRelayControl();
void addCorsHeaders(WebServer* server);

// Helper functions
String getNetworksJson();
void setupWebHandlers();



