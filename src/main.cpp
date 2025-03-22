#include <Arduino.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <time.h>
#include "GlobalState.h"
#include "DisplayHandler.h"
#include "BME280Handler.h"
#include "MQTTManager.h"
#include <ESPmDNS.h>
#include "WebServerManager.h"
#include "config.h"
#include "PreferencesManager.h"
#include "RelayControlHandler.h" 
#include "WebHandlers.h"
#include "BabelSensor.h"
#include "SystemMonitor.h"
#include "TaskManager.h"
#include <esp_wifi.h>
#include "WiFiConnectionManager.h"

// System Constants
constexpr uint32_t BOOT_DELAY_MS = 250;
constexpr uint32_t WIFI_TIMEOUT_MS = 10000;
constexpr uint32_t WDT_TIMEOUT_S = 60;  // Increased from 30 to 60 for better stability
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr uint32_t NETWORK_TASK_STACK_SIZE = 8192;  // Double stack for network tasks
constexpr uint32_t QUEUE_SIZE = 10;
constexpr uint32_t WIFI_RECONNECT_INTERVAL = 30000;
constexpr uint32_t MEMORY_CHECK_INTERVAL = 10000;   // Check memory every 10 seconds
constexpr uint32_t REMOTE_TEMP_UPDATE_INTERVAL = 30000;
constexpr uint32_t STACK_CHECK_INTERVAL = 300000;   // Check task stacks every 5 minutes
constexpr uint32_t MQTT_RETRY_LIMIT = 5;            // Maximum MQTT connection attempts before timeout
constexpr uint32_t DISCOVERY_INTERVAL = 3600000;    // HomeAssistant discovery interval - 1 hour
static unsigned long lastNtpSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 1200000; 


// Global objects
MQTTManager mqttManager;
SystemMonitor sysMonitor(&mqttManager);
RelayControlHandler* g_relayHandler = nullptr;
BabelSensor babelSensor(SENSORHUB_URL);
static DisplayHandler* display = nullptr;
static BME280Handler bme280;

// Network status tracking
enum class NetworkStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    PORTAL_ACTIVE
};

// Global state variables
NetworkStatus networkStatus = NetworkStatus::DISCONNECTED;
bool mqttInitialized = false;
bool ntpInitialized = false;
bool webServerInitialized = false;
static unsigned long lastDiscoveryAttempt = 0;
static bool initialDiscoveryDone = false;
static unsigned long lastWdtReset = 0;
static unsigned long lastRemoteTempUpdate = 0;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastMemoryCheck = 0;
static unsigned long lastStackCheck = 0;
static float lastBabelTemp = 0.0;
static uint32_t minHeapSeen = UINT32_MAX;
static uint8_t mqttReconnectCount = 0;

// Store device ID globally for mDNS
char deviceIdString[5] = {0};

// External declarations from GlobalDefinitions.cpp
extern GlobalState* g_state;
extern QueueHandle_t displayQueue;
extern QueueHandle_t sensorQueue;
extern TaskHandle_t displayTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern TaskHandle_t watchdogTaskHandle;

// Task Declarations
void displayTask(void* parameter);
void sensorTask(void* parameter);
void networkTask(void* parameter);
void publishRelayState(uint8_t relayId, RelayState state, RelayCommandSource source);

// Function Declarations
void displayDeviceId();
void initializeDisplayPreferences();
bool initializeSystem();
void setupRelayControl();
bool setupMDNS();
bool setupNTP();
void initializeMQTT();
bool initializeWebServerManager();
bool setupNetwork();
void monitorNetwork();
void createTasks();
void checkHeapFragmentation();
void monitorTaskStacks();

// --------------------------
// Main System Initialization
// --------------------------
void startPortalMode() {
    Serial.println("Starting WiFi portal mode...");
    
    // Initialize WebServerManager if needed
    if (!webServerInitialized) {
        initializeWebServerManager();
    }
    
    // Start portal mode
    auto& webManager = WebServerManager::getInstance();
    if (webManager.startPortalMode()) {
        Serial.println("Portal mode started successfully");
        networkStatus = NetworkStatus::PORTAL_ACTIVE;
        
        // Show "AP" on display
        if (display) {
            display->setDigit(0, CHAR_A);
            display->setDigit(1, CHAR_P);
            display->setDigit(2, CHAR_BLANK);
            display->setDigit(3, CHAR_BLANK);
            display->update();
        }
    } else {
        Serial.println("Failed to start portal mode");
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n===========================");
    Serial.println("System starting...");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.println("===========================\n");

    // Initialize system components
    if (!initializeSystem()) {
        Serial.println("Critical: System initialization failed");
        delay(1000);
        ESP.restart();
    }

    // Initialize the WiFi connection manager
    auto& wifiManager = WiFiConnectionManager::getInstance();
    if (!wifiManager.begin()) {
        Serial.println("Warning: WiFi Connection Manager initialization failed");
    }
    
    // Register connection status callback
    wifiManager.setStatusCallback([](WiFiStatus status, String ipAddress) {
        switch (status) {
            case WiFiStatus::DISCONNECTED:
                Serial.println("WiFi Status Changed: DISCONNECTED");
                networkStatus = NetworkStatus::DISCONNECTED;
                break;
                
            case WiFiStatus::CONNECTING:
                Serial.println("WiFi Status Changed: CONNECTING");
                networkStatus = NetworkStatus::CONNECTING;
                break;
                
            case WiFiStatus::CONNECTED:
                Serial.printf("WiFi Status Changed: CONNECTED (IP: %s)\n", ipAddress.c_str());
                networkStatus = NetworkStatus::CONNECTED;
                
                // Setup MDNS 
                if (!setupMDNS()) {
                    Serial.println("Warning: mDNS setup failed");
                }
                
                // Setup NTP now that we have connectivity
                if (setupNTP()) {
                    Serial.println("NTP synchronization successful");
                    lastNtpSync = millis();
                } else {
                    Serial.println("NTP synchronization failed, will retry later");
                    lastNtpSync = millis() - NTP_SYNC_INTERVAL + 60000; // Retry in 1 minute
                }
                
                // Initialize MQTT now that we're connected
                if (!mqttInitialized) {
                    initializeMQTT();
                }
                break;
                
            case WiFiStatus::CONNECTION_FAILED:
                Serial.println("WiFi Status Changed: CONNECTION_FAILED");
                networkStatus = NetworkStatus::DISCONNECTED;
                // Start portal mode if connection failed repeatedly
                startPortalMode();
                break;
                
            case WiFiStatus::PORTAL_ACTIVE:
                Serial.println("WiFi Status Changed: PORTAL_ACTIVE");
                networkStatus = NetworkStatus::PORTAL_ACTIVE;
                break;
        }
    });
    
    // Display initialization screen
    if (display) {
        display->setDigit(0, CHAR_BLANK);
        display->setDigit(1, CHAR_BLANK);
        display->setDigit(2, CHAR_BLANK);
        display->setDigit(3, CHAR_BLANK);
        display->update();
        delay(500);
        
        display->setDigit(0, CHAR_I);
        display->setDigit(1, CHAR_n);
        display->setDigit(2, CHAR_I);
        display->setDigit(3, CHAR_t);
        display->update();
    }

    // First try to connect to WiFi
    if (wifiManager.hasStoredCredentials()) {
        Serial.println("Connecting to WiFi with stored credentials...");
        
        // Try to connect
        if (wifiManager.connectWithStoredCredentials()) {
            Serial.println("WiFi connection established successfully");
            // Initialize WebServer in normal mode
            initializeWebServerManager();
            // Note: Other network-dependent services are now initialized in the callback
        } else {
            Serial.println("Failed to connect with stored credentials");
            // Start portal mode
            startPortalMode();
        }
    } else {
        Serial.println("No stored WiFi credentials found");
        // Start portal mode
        startPortalMode();
    }

    // Initialize BME280 sensor (works with or without WiFi)
    if (!bme280.init()) {
        Serial.println("Warning: BME280 initialization failed");
        g_state->setBMEWorking(false);
    } else {
        g_state->setBMEWorking(true);
        Serial.println("BME280 initialized successfully");
    }

    // Setup watchdog with extended timeout for stability
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    // Create system tasks - this happens regardless of WiFi status
    createTasks();
    
    // Start system monitoring
    sysMonitor.begin();
    
    Serial.println("Setup complete");
}



// --------------------------
// Main Loop
// --------------------------

void loop() {
    const unsigned long now = millis();

    // Watchdog handling - guarantee we reset watchdog regularly
    if (now - lastWdtReset >= 1000) {
        esp_task_wdt_reset();
        lastWdtReset = now;
    }
    
    // Monitor and manage network connectivity
    monitorNetwork();

    // Explicit MQTT reconnection handling
    if (networkStatus == NetworkStatus::CONNECTED && mqttInitialized) {
        static unsigned long lastMqttCheck = 0;
        unsigned long now = millis();
        
        if (now - lastMqttCheck >= 10000) { // Check every 10 seconds
            lastMqttCheck = now;
            
            if (!mqttManager.connected()) {
                Serial.println("[MAIN] MQTT not connected, forcing reconnection attempt");
                
                // Force fresh reconnection attempt
                bool reconnectResult = mqttManager.connect();
                if (reconnectResult) {
                    Serial.println("[MAIN] MQTT reconnection successful from main loop");
                } else {
                    Serial.println("[MAIN] MQTT reconnection failed from main loop");
                }
            } else {
                // Silently verify connection is working
                mqttManager.loop();
            }
        }
    }
    

    // Update remote temperature at fixed interval if network is up
    if (networkStatus == NetworkStatus::CONNECTED && now - lastRemoteTempUpdate >= REMOTE_TEMP_UPDATE_INTERVAL) {
        // Check if sensor is enabled before attempting to get temperature
        if (babelSensor.isEnabled()) {
            float remoteTemp = babelSensor.getRemoteTemperature();
            if (remoteTemp != 0.0 && remoteTemp != lastBabelTemp) {
                g_state->setRemoteTemperature(remoteTemp);
                lastBabelTemp = remoteTemp;
                Serial.printf("Updated remote temperature: %.2f°C\n", remoteTemp);
            } else {
                Serial.println("Remote temperature unchanged or invalid");
            }
        } else {
            // Try to initialize if not already enabled but should be
            DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
            if (prefs.useSensorhub && !babelSensor.isEnabled()) {
                Serial.println("BabelSensor should be enabled - reinitializing");
                babelSensor.init();
            }
        }
        lastRemoteTempUpdate = now;
    }
    
    // Check memory and heap fragmentation periodically
    if (now - lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
        checkHeapFragmentation();
        lastMemoryCheck = now;
    }
    
    // Update system diagnostics
    sysMonitor.update();
    
    // Monitor task stacks periodically
    if (now - lastStackCheck >= STACK_CHECK_INTERVAL) {
        monitorTaskStacks();
        lastStackCheck = now;
    }
    
    // Home Assistant Discovery Management
    // First check if we need to do the initial discovery
    if (!initialDiscoveryDone && mqttManager.connected()) {
        Serial.println("Publishing initial Home Assistant discovery");
        // Using a simplified discovery approach to reduce memory usage
        mqttManager.publishSensorDiscovery("temperature", "°C", "temperature");
        delay(500);
        mqttManager.publishSensorDiscovery("humidity", "%", "humidity");
        
        initialDiscoveryDone = true;
        lastDiscoveryAttempt = now;
    }
    // Periodic re-discovery to ensure entities stay available
    else if (initialDiscoveryDone && now - lastDiscoveryAttempt > DISCOVERY_INTERVAL) {
        if (mqttManager.connected()) {
            Serial.println("Publishing periodic Home Assistant discovery refresh");
            mqttManager.publishSensorDiscovery("temperature", "°C", "temperature");
            delay(500);
            mqttManager.publishSensorDiscovery("humidity", "%", "humidity");
        }
        lastDiscoveryAttempt = now;
    }
    
    if (networkStatus == NetworkStatus::CONNECTED) {
        unsigned long now = millis();
        if (now - lastNtpSync >= NTP_SYNC_INTERVAL) {
            Serial.println("[NTP] Resynchronizing NTP time");
            bool ntpSuccess = setupNTP();
            
            // Record the attempt in SystemMonitor
            sysMonitor.recordNtpSyncAttempt(ntpSuccess);
            
            if (ntpSuccess) {
                Serial.println("[NTP] Time resynchronized successfully");
                lastNtpSync = now;
            } else {
                Serial.println("[NTP] Resynchronization failed");
                // Retry sooner if failed
                lastNtpSync = now - (NTP_SYNC_INTERVAL - 300000); // Try again in 5 minutes
            }
        }
    }
    // Short yield to give other tasks time to execute
    delay(10);
}

// --------------------------
// System Functions
// --------------------------

void checkHeapFragmentation() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapSize = ESP.getHeapSize();
    uint8_t fragmentation = 100 - (freeHeap * 100) / heapSize;
    
    if (freeHeap < minHeapSeen) {
        minHeapSeen = freeHeap;
    }
    
    Serial.printf("Heap - Free: %u bytes, Min: %u, Fragmentation: %u%%\n", 
                 freeHeap, minHeapSeen, fragmentation);
    
    // Memory warning thresholds
    if (fragmentation > 70) {
        Serial.println("WARNING: Critical heap fragmentation detected!");
    }
    
    if (freeHeap < 20000) {
        Serial.println("WARNING: Low memory condition detected!");
        
        // Recovery actions for low memory
        if (mqttManager.connected()) {
            mqttManager.forceDisconnect();
            Serial.println("MQTT disconnected to free memory");
            delay(200);
            mqttManager.connect();  // Try to reconnect with fresh state
        }
    }
}

void monitorTaskStacks() {
    Serial.println("Task stack high water marks:");
    
    if (displayTaskHandle) {
        Serial.printf("Display task: %u bytes remaining\n", 
                      uxTaskGetStackHighWaterMark(displayTaskHandle));
    }
    
    if (sensorTaskHandle) {
        Serial.printf("Sensor task: %u bytes remaining\n", 
                      uxTaskGetStackHighWaterMark(sensorTaskHandle));
    }
    
    if (networkTaskHandle) {
        Serial.printf("Network task: %u bytes remaining\n", 
                      uxTaskGetStackHighWaterMark(networkTaskHandle));
    }
    
    if (watchdogTaskHandle) {
        Serial.printf("Watchdog task: %u bytes remaining\n", 
                      uxTaskGetStackHighWaterMark(watchdogTaskHandle));
    }

    // If any stack is dangerously low, log a warning
    if ((displayTaskHandle && uxTaskGetStackHighWaterMark(displayTaskHandle) < 256) ||
        (sensorTaskHandle && uxTaskGetStackHighWaterMark(sensorTaskHandle) < 256) ||
        (networkTaskHandle && uxTaskGetStackHighWaterMark(networkTaskHandle) < 256) ||
        (watchdogTaskHandle && uxTaskGetStackHighWaterMark(watchdogTaskHandle) < 256)) {
        Serial.println("WARNING: Critically low task stack detected!");
    }
    
    // Also publish to MQTT if connected
    if (mqttInitialized && mqttManager.connected()) {
        TaskHandle_t taskHandles[] = {displayTaskHandle, sensorTaskHandle, networkTaskHandle, watchdogTaskHandle};
        const char* taskNames[] = {"display", "sensor", "network", "watchdog"};
        sysMonitor.monitorTaskStacks(taskHandles, taskNames, 4);
    }
}

bool initializeSystem() {
    // Mount file system
    if (!SPIFFS.begin(true)) {
        Serial.println("Critical: SPIFFS mount failed");
        return false;
    }
    Serial.println("SPIFFS mounted successfully");

    // Initialize Global State
    g_state = &GlobalState::getInstance();
    if (!g_state) {
        Serial.println("Critical: Failed to initialize global state");
        return false;
    }

    // Create System Queues
    displayQueue = xQueueCreate(QUEUE_SIZE, sizeof(DisplayMode));
    sensorQueue = xQueueCreate(QUEUE_SIZE, sizeof(BME280Data));
    if (!displayQueue || !sensorQueue) {
        Serial.println("Critical: Queue creation failed");
        return false;
    }

    // Initialize Display
    display = new DisplayHandler();
    if (!display || !display->init()) {
        Serial.println("Critical: Display initialization failed");
        return false;
    }
    g_state->setDisplay(display);
   
    // Initialize I2C bus for sensors
    if (!Wire.begin(I2C_SDA, I2C_SCL, 100000)) {
        Serial.println("Critical: I2C initialization failed");
        return false;
    }
    delay(BOOT_DELAY_MS);

    // Display device ID 
    displayDeviceId();

    // Initialize preferences and apply them
    PreferencesManager::begin();
  
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    if (prefs.useSensorhub) {
        if (babelSensor.init()) {
            Serial.println("BabelSensor initialized successfully");
        } else {
            Serial.println("BabelSensor initialization failed");
        }
    }
    
    initializeDisplayPreferences();
    return true;
}

void initializeDisplayPreferences() {
    Serial.println("[INIT] Loading display preferences from storage");
    
    // Load preferences
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    // Log loaded preferences
    Serial.printf("[INIT] Loaded preferences: Night Mode=%s, Day=%d%%, Night=%d%%, Start=%d:00, End=%d:00\n",
                 prefs.nightModeDimmingEnabled ? "Enabled" : "Disabled",
                 prefs.dayBrightness,
                 prefs.nightBrightness,
                 prefs.nightStartHour,
                 prefs.nightEndHour);
    
    // Apply preferences to display
    DisplayHandler* display = g_state->getDisplay();
    if (display) {
        Serial.println("[INIT] Applying saved preferences to display");
        display->setDisplayPreferences(prefs);
    } else {
        Serial.println("[ERROR] Cannot apply preferences - display not initialized");
    }
}

void displayDeviceId() {
    if (!display || !g_state || !g_state->getDisplay()) {
        Serial.println("[ERROR] Cannot display ID - display not initialized");
        return;
    }
    
    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    int identifier = (mac[4] << 8) | mac[5];  
    snprintf(deviceIdString, sizeof(deviceIdString), "%04X", identifier);
    
    Serial.printf("[INIT] Device identifier: %s\n", deviceIdString);
    
    // Set high brightness for ID display
    uint8_t currentBrightness = 75;
    display->setBrightness(currentBrightness);
    
    // Show identifier for about 8 seconds with walking dot
    const int DOT_INTERVAL_MS = 400;
    const int TOTAL_DISPLAY_TIME = 8000;
    unsigned long startTime = millis();
    int currentDot = 0;
    
    // Convert and show the ID digits
    for(int i = 0; i < 4; i++) {
        char c = deviceIdString[i];
        int digit;
        if(c >= '0' && c <= '9') {
            digit = c - '0' + CHAR_0;
        } else {
            digit = c - 'A' + CHAR_A;
        }
        display->setDigit(i, digit, false);
    }
    
    // Keep updating the display with walking dot
    while(millis() - startTime < TOTAL_DISPLAY_TIME) {
        currentDot = ((millis() - startTime) / DOT_INTERVAL_MS) % 4;
        
        for(int i = 0; i < 4; i++) {
            char c = deviceIdString[i];
            int digit;
            if(c >= '0' && c <= '9') {
                digit = c - '0' + CHAR_0;
            } else {
                digit = c - 'A' + CHAR_A;
            }
            display->setDigit(i, digit, (i == currentDot));
        }
        
        display->update();
        delay(50);
    }
    
    Serial.println("[INIT] ID display complete");
}

void setupRelayControl() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) {
        Serial.println("[ERROR] Cannot set up relay control - server not initialized");
        return;
    }

    // Initialize relay handler instance
    g_relayHandler = &RelayControlHandler::getInstance();
    if (!g_relayHandler->begin()) {
        Serial.println("Failed to initialize relay control");
        return;
    }
    
    // Register relay web handlers
    server->on("/api/relay", HTTP_GET, handleGetRelayState);
    server->on("/api/relay", HTTP_POST, handleSetRelayState);
    
    Serial.println("Relay control initialized successfully");
}

bool setupMDNS() {
    // Use MQTT_CLIENT_ID directly as the mDNS name
    String mdnsName = String(MQTT_CLIENT_ID);
    
    Serial.printf("Setting up mDNS with name: %s\n", mdnsName.c_str());
    
    if (!MDNS.begin(mdnsName.c_str())) {
        Serial.println("Error setting up mDNS responder!");
        return false;
    } else {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS responder started. Device will be accessible at %s.local\n", mdnsName.c_str());
        return true;
    }
}

bool setupNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] Cannot set up NTP - WiFi not connected");
        return false;
    }
    
    // Setup time
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_INFO, 1);
    tzset();
    
    // Check if time was set successfully
    struct tm timeinfo;
    unsigned long startAttempt = millis();
    while (!getLocalTime(&timeinfo) && millis() - startAttempt < 5000) {
        delay(100);
    }
    
    if (getLocalTime(&timeinfo)) {
        Serial.printf("NTP time set: %02d:%02d:%02d\n", 
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        ntpInitialized = true;
        return true;
    } else {
        Serial.println("Warning: Failed to set time via NTP");
        return false;
    }
}

void initializeMQTT() {
    if (mqttInitialized) return;
    
    // Use the parameter-less begin() method which will load preferences directly
    if (mqttManager.begin()) {
        mqttInitialized = true;
        
        // Register relay callback
        mqttManager.setCallback([](char* topic, uint8_t* payload, unsigned int length) {
            // Convert payload to String for easier handling
            String payloadStr((char*)payload, length);
            RelayControlHandler::handleMqttMessage(topic, payloadStr);
        });
        
        Serial.println("MQTT initialized successfully");
    } else {
        Serial.println("MQTT initialization failed - will retry later");
    }
}

bool initializeWebServerManager() {
    if (webServerInitialized) return true;
    
    // Initialize WebServerManager
    auto& webManager = WebServerManager::getInstance();
    if (!webManager.begin()) {
        Serial.println("Failed to initialize WebServerManager");
        return false;
    }
    
    webServerInitialized = true;
    Serial.println("WebServerManager initialized successfully");
    return true;
}

bool setupNetwork() {
    // First, ensure WebServerManager is initialized
    if (!initializeWebServerManager()) {
        Serial.println("Critical: Failed to initialize WebServerManager");
        return false;
    }
    
    auto& webManager = WebServerManager::getInstance();
    
    // Check if WiFi credentials exist and try to connect
    if (webManager.hasStoredCredentials()) {
        networkStatus = NetworkStatus::CONNECTING;
        Serial.println("Connecting to WiFi with stored credentials...");
        
        // More robust WiFi configuration
        WiFi.disconnect(true, true);  // Reset Wi-Fi configuration
        WiFi.setHostname(MQTT_CLIENT_ID);
        WiFi.mode(WIFI_STA);
        Serial.printf("[WIFI] Setting hostname to: %s\n", MQTT_CLIENT_ID);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        
        // Disable power saving mode for better stability
        esp_wifi_set_ps(WIFI_PS_NONE);
        
        WiFi.begin();

        unsigned long startAttempt = millis();
        unsigned long connectionTimeout = 20000; // Increased from 10 seconds to 20
        
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startAttempt > connectionTimeout) {
                Serial.println("WiFi connection timeout - starting setup portal");
                networkStatus = NetworkStatus::PORTAL_ACTIVE;
                return webManager.startPortalMode();
            }
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        // Extra delay after connection to ensure DHCP completes
        delay(1000);
        
        // Wait specifically for IP address
        startAttempt = millis();
        while (WiFi.localIP().toString() == "0.0.0.0") {
            if (millis() - startAttempt > 5000) {
                Serial.println("DHCP timeout - failed to get IP address");
                networkStatus = NetworkStatus::PORTAL_ACTIVE;
                return webManager.startPortalMode();
            }
            delay(500);
            Serial.print("D");
        }
        Serial.println();

        // WiFi connected successfully
        networkStatus = NetworkStatus::CONNECTED;
        Serial.printf("WiFi connected successfully. IP address: %s\n", WiFi.localIP().toString().c_str());
        
        // Set up mDNS
        setupMDNS();
        
        // Initialize WebServerManager in normal mode
        Serial.println("Starting WebServerManager in normal mode...");
        if (!webManager.startPreferencesMode()) {
            Serial.println("Failed to start WebServerManager in normal mode");
            return false;
        }
        
        // Setup NTP time service
        setupNTP();
        
        return true;
    } else {
        // No credentials exist, start in portal mode
        Serial.println("No WiFi credentials found - starting in setup portal mode");
        networkStatus = NetworkStatus::PORTAL_ACTIVE;
        return webManager.startPortalMode();
    }
}

void monitorNetwork() {
    const unsigned long now = millis();
    auto& webManager = WebServerManager::getInstance();
    
    // Ensure WebServerManager is initialized
    if (!webServerInitialized && !initializeWebServerManager()) {
        Serial.println("[ERROR] Unable to initialize WebServerManager");
        return;
    }
    
    // Skip if we're in portal mode already
    if (networkStatus == NetworkStatus::PORTAL_ACTIVE) {
        // Just handle the portal clients
        if (webManager.getServer()) {
            webManager.handleClient();
        } else {
            Serial.println("[WARNING] Server is null in portal mode - attempting to restart portal");
            webManager.startPortalMode();
        }
        return;
    }
    
    // Check if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        if (networkStatus != NetworkStatus::CONNECTED) {
            Serial.println("WiFi reconnected");
            networkStatus = NetworkStatus::CONNECTED;
            
            // If we just reconnected, ensure MQTT is initialized
            if (!mqttInitialized) {
                initializeMQTT();
            }
            
            // Try to set up NTP if it hasn't been done yet
            if (!ntpInitialized) {
                setupNTP();
            }
            
            // Ensure WebServerManager is running in the correct mode
            if (webManager.getCurrentMode() != WebServerManager::ServerMode::PREFERENCES) {
                Serial.println("Restarting WebServerManager in normal mode");
                webManager.startPreferencesMode();
            }
        }
        
        // Handle normal web server requests
        webManager.handleClient();
        
        // Maintain MQTT connection if initialized
        if (mqttInitialized) {
            mqttManager.maintainConnection();
        }
    } else {
        // WiFi disconnected, attempt reconnection periodically
        if (networkStatus != NetworkStatus::DISCONNECTED) {
            Serial.println("WiFi connection lost");
            networkStatus = NetworkStatus::DISCONNECTED;
            
            // Reset MQTT connection attempt counter when WiFi drops
            mqttReconnectCount = 0;
        }
        
        if (now - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
            Serial.println("Attempting WiFi reconnection");
            
            // Try to reconnect using stored credentials
            if (webManager.hasStoredCredentials()) {
                networkStatus = NetworkStatus::CONNECTING;
                
                // Attempt reconnection
                if (WiFi.reconnect()) {
                    Serial.println("WiFi reconnection started");
                } else {
                    Serial.println("WiFi reconnection failed - starting portal mode");
                    networkStatus = NetworkStatus::PORTAL_ACTIVE;
                    webManager.startPortalMode();
                }
            } else {
                // No credentials, start portal mode
                Serial.println("No stored credentials - starting portal mode");
                networkStatus = NetworkStatus::PORTAL_ACTIVE;
                webManager.startPortalMode();
            }
            
            lastReconnectAttempt = now;
        }
    }
}

void createTasks() {
    // Display task on core 1 for consistent timing
    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        TASK_STACK_SIZE,
        nullptr,
        1,
        &displayTaskHandle,
        1
    );

    // Sensor task on core 0
    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        TASK_STACK_SIZE,
        nullptr,
        1,
        &sensorTaskHandle,
        0
    );
    
    // Network task on core 0 with larger stack size
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        NETWORK_TASK_STACK_SIZE,
        nullptr,
        1,
        &networkTaskHandle,
        0
    );
}

// --------------------------
// FreeRTOS Tasks
// --------------------------

void networkTask(void* parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // Check every second
    
    while (true) {
        esp_task_wdt_reset();
        
        // This task currently only serves to keep the watchdog fed
        // And to provide separate stack space for network operations
        
        vTaskDelay(xDelay);
    }
}

void displayTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(100);  // 10Hz refresh rate
    struct tm timeinfo;
    DisplayMode mode = DisplayMode::TIME;
    
    // Mutex health check variables
    unsigned long lastMutexCheck = 0;
    const unsigned long MUTEX_CHECK_INTERVAL = 30000; // 30 seconds
    
    while (true) {
        esp_task_wdt_reset();
        unsigned long now = millis();

        // Periodic mutex health check
        if (now - lastMutexCheck >= MUTEX_CHECK_INTERVAL) {
            if (display && !display->isMutexValid()) {
                Serial.println("[CRITICAL ERROR] Display mutex invalid in task!");
            }
            lastMutexCheck = now;
        }

        // Update display mode if requested via queue
        if (xQueueReceive(displayQueue, &mode, 0) == pdTRUE) {
            display->setMode(mode);
        }

        // Get current mode
        DisplayMode currentMode = display->getCurrentMode();
        
        // Update display based on current mode
        switch(currentMode) {
            case DisplayMode::TIME:
                if (getLocalTime(&timeinfo)) {
                    display->showTime(timeinfo.tm_hour, timeinfo.tm_min);
                } else if (networkStatus == NetworkStatus::PORTAL_ACTIVE) {
                    // Show "AP" for Access Point mode when time is not available
                    display->setDigit(0, CHAR_A);
                    display->setDigit(1, CHAR_P);
                    display->setDigit(2, CHAR_BLANK);
                    display->setDigit(3, CHAR_BLANK);
                }
                break;
            case DisplayMode::DATE:
                if (getLocalTime(&timeinfo)) {
                    display->showDate(timeinfo.tm_mday, timeinfo.tm_mon + 1);
                }
                break;
            case DisplayMode::TEMPERATURE:
                display->showTemperature(g_state->getTemperature());
                break;
            case DisplayMode::HUMIDITY:
                display->showHumidity(g_state->getHumidity());
                break;
            case DisplayMode::PRESSURE:
                display->showPressure(g_state->getPressure());
                break;
            case DisplayMode::REMOTE_TEMP:
                display->showRemoteTemp(g_state->getRemoteTemperature());
                break;
        }

        display->update();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void publishRelayState(uint8_t relayId, RelayState state, RelayCommandSource source) {
    if (mqttInitialized && mqttManager.connected() && networkStatus == NetworkStatus::CONNECTED) {
        StaticJsonDocument<128> stateDoc;
        stateDoc["relay_id"] = relayId;
        stateDoc["state"] = (state == RelayState::ON);
        
        // Include the source of the state change
        switch (source) {
            case RelayCommandSource::USER:
                stateDoc["source"] = "user";
                break;
            case RelayCommandSource::MQTT:
                stateDoc["source"] = "mqtt";
                break;
            default:
                stateDoc["source"] = "system";
                break;
        }
        
        String statePayload;
        serializeJson(stateDoc, statePayload);
        
        // Use consistent topic construction with MAC-based ID for uniqueness
        String clientId = "ablutionoracle_" + WiFi.macAddress().substring(9);
        clientId.replace(":", "");
        String stateTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_RELAY) + "/state";
        
        mqttManager.publish(stateTopic.c_str(), statePayload.c_str(), true);
    }
}

void sensorTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000);  // 0.5Hz measurement rate
    unsigned long lastStatusPublish = 0;
    const unsigned long STATUS_PUBLISH_INTERVAL = 60000;  // Publish status every minute (reduced from 5 min)
    
    while (true) {
        esp_task_wdt_reset();
        unsigned long now = millis();

        // Take sensor measurements if BME280 is working
        if (g_state->isBMEWorking() && bme280.takeMeasurement()) {
            float temperature = bme280.getTemperature();
            float humidity = bme280.getHumidity();
            float pressure = bme280.getPressure();

            // Validate readings before using them
            if (temperature != BME280_INVALID_TEMP && 
                humidity != BME280_INVALID_HUM && 
                pressure != BME280_INVALID_PRES) {
                
                g_state->updateSensorData(temperature, humidity, pressure);
                
                // Only publish to MQTT if connected
                if (mqttInitialized && mqttManager.connected() && networkStatus == NetworkStatus::CONNECTED) {
                    // Ensure we publish a status more frequently
                    String statusTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/sensor/status";
                    
                    if (now - lastStatusPublish >= STATUS_PUBLISH_INTERVAL) {
                        if (mqttManager.publish(statusTopic.c_str(), "online", true)) {
                            Serial.println("Published status: online (retained)");
                            lastStatusPublish = now;
                        } else {
                            Serial.println("Failed to publish status, will retry sooner");
                            lastStatusPublish = now - STATUS_PUBLISH_INTERVAL + 10000; // Retry in 10 seconds
                        }
                    }
                    
                    // Format the values with 1 decimal place precision
                    char tempStr[8], humStr[8], presStr[8];
                    snprintf(tempStr, sizeof(tempStr), "%.1f", temperature);
                    snprintf(humStr, sizeof(humStr), "%.1f", humidity);
                    snprintf(presStr, sizeof(presStr), "%.1f", pressure);
                    
                    // Create the JSON string manually to ensure exact formatting
                    char payload[192];
                    snprintf(payload, sizeof(payload), "{\"temperature\":%s,\"humidity\":%s,\"pressure\":%s}", 
                            tempStr, humStr, presStr);
                    
                    // Publish to MQTT using the EXACT same topic format as in the discovery
                    String sensorTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/sensor/sensors";
                    
                    if (!mqttManager.publish(sensorTopic.c_str(), payload)) {
                        Serial.println("Failed to publish sensor data, will retry next cycle");
                    } else {
                        Serial.println("Successfully published sensor data");
                    }
                }
            } else {
                Serial.println("Invalid sensor readings, skipping publication");
            }
        }
        
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}