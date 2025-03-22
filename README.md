# A Smart Clock That Does More Than Just Tell Time

It all started with a cold shower—literally. After a few unpleasant surprises in the morning, I figured it would be useful to know before stepping in whether hot water was actually available elsewhere in the building. What began as a simple idea quickly spiraled (as these things tend to do) into a full-blown project.

The result? A smart and versatile clock that does more than just display the time. Alongside the current time and date, it also shows real-time temperature, humidity, and air pressure—both locally and from a remote sensor. Data is sent via an MQTT broker, but if the broker is unavailable, the sensorHUB reads the remote sensor directly. In other words, you’ll always get reliable readings, no matter what.

Under the hood, the system runs on FreeRTOS with a watchdog and task monitoring to keep things stable and robust. At one point, I considered adding news headlines to the display, but my housemates vetoed that idea—apparently, it was too much first thing in the morning.

Want to build one yourself? Or just curious about the code and design? Check out this project on GitHub! 

System architecture:
```mermaid
flowchart TD
    %% System Initialization
    A[Power On] --> B[Initialize Hardware]
    B --> C{Has WiFi Credentials?}
    C -->|Yes| D[Connect to WiFi]
    C -->|No| E[Start Portal Mode]
    D -->|Success| F[Setup mDNS & NTP]
    D -->|Failure| E
    F --> G[Initialize MQTT]
    G --> H[Initialize WebServer]
    E --> H
    
    %% Main Tasks
    H --> I[Create FreeRTOS Tasks]
    I --> J[Display Task]
    I --> K[Sensor Task]
    I --> L[Network Task]
    I --> M[System Monitor]
    
    %% Component Interactions
    N[WiFi Manager] --- O[MQTT Manager]
    P[WebServer Manager] --- Q[NTP Sync]
    
    R[BME280 Sensor] --> K
    K --> U[Global State]
    Q --> U
    U --> J
    J --> S[7-Segment Display]
    P --> T[Relay Control]
    O --> T
```


## Why This Codebase is Unique
- **Modular Design:** All functions are split into separate files (e.g., MQTTManager, BME280Handler, DisplayHandler) to ease maintainability and reusability.
- **Robust Connectivity:** Uses secure SSL connections to connect to the MQTT broker with built-in reconnection logic.
- **Dual Sensor Integration:** Supports both locally attached sensors and remote sensor data, offering flexibility in deployment.
- **Real-Time Tasks:** Implements multi-tasking using FreeRTOS, ensuring smooth operations even under heavy load.
- **OTA Friendly:** Ready for over-the-air (OTA) updates, making field upgrades seamless.
  
## Installation and Setup

### Prerequisites
- **Platform:** ESP32 development board. I used AZ-Deliviry ESP32 DEV C4.
- **Toolchain:** [PlatformIO](https://platformio.org/) or Arduino IDE with ESP32 support installed.
- **Libraries:**  
  - **Arduino.h:** Core Arduino library for ESP32.
  - **SPIFFS:** Filesystem library to manage onboard flash storage.
  - **Wire:** I2C communication for sensor interfacing.
  - **WiFi:** Library to manage WiFi connections.
  - **esp_task_wdt & esp_system:** For watchdog timer and system management.
  - **Time.h:** To manage NTP synchronization.
  - **Custom libraries:** GlobalState, DisplayHandler, BME280Handler, MQTTManager (found under `include/` and `src/` folders).
  
### Setup Instructions for a Fresh Compile
1. **Clone the Repository:**
   ```bash
   git clone https://github.com/yourusername/esp32-ota-test.git
   cd esp32-ota-test
   ```
2. **Configure PlatformIO or Arduino IDE:**
   - For **PlatformIO**: Open the project in [VS Code with PlatformIO extension](https://platformio.org/).
   - For **Arduino IDE**: Ensure ESP32 board support is installed via the Board Manager.
3. **Set Credentials:**
   - Edit the `config.h` file to update MQTT broker credentials, NTP server, timezone (TZ_INFO), and other configuration details.
4. **Build and Upload:**
   - With PlatformIO, run the build task and then upload the firmware to your ESP32 board.
   - With Arduino IDE, select the correct board/port and click Upload.

5. **Monitor as It Runs:**
   - Open the serial monitor (set to 115200 baud) to observe system initialization, connectivity logs, and sensor data publishing.

## Software architecture
```mermaid
classDiagram
    GlobalState --> DisplayHandler : passes data to
    GlobalState --> MQTTManager : provides data for
    DisplayHandler --> DisplayMode : implements
    
    PreferencesManager --> DisplayPreferences : manages
    DisplayHandler --> DisplayPreferences : uses
    
    BME280Handler --> GlobalState : updates
    WiFiConnectionManager --> GlobalState : updates status
    
    BabelSensor --> GlobalState : updates remote temp
    RelayControlHandler --> MQTTManager : publishes via
    
    WebServerManager --> WiFiConnectionManager : uses
    SystemMonitor --> MQTTManager : publishes via
    
    class GlobalState {
        +temperature
        +humidity
        +pressure
        +remoteTemperature
        +display
        +updateSensorData()
        +setRemoteTemperature()
    }
    
    class DisplayHandler {
        +showTime()
        +showDate()
        +showTemperature()
        +showHumidity()
        +showPressure()
        +update()
    }
    
    class PreferencesManager {
        +loadDisplayPreferences()
        +saveDisplayPreferences()
    }
    
    class WiFiConnectionManager {
        +connect()
        +disconnect()
        +isConnected()
    }
    
    class MQTTManager {
        +publish()
        +subscribe()
        +connected()
    }
    
    class WebServerManager {
        +startPortalMode()
        +startPreferencesMode()
        +handleClient()
    }
```


## Data Flow Diagram
```mermaid
flowchart LR
    %% Physical I/O Section
    BME[BME280 Sensor] --> BMEHandler[BME280 Handler]
    DisplayH[Display Handler] --> Display[7-Segment Display]
    Button[Input Buttons] --> DisplayH
    
    %% Sensor Processing Section
    BMEHandler --> GState[Global State]
    BabelS[BabelSensor] --> GState
    
    %% State Management Section
    GState --> DisplayH
    GState --> MQTT_P[MQTT Publisher]
    
    %% Display Management Section
    Timer[System Time] --> DisplayH
    
    %% Network Communications Section
    WiFiMgr[WiFi Manager] <--> Internet[Internet]
    MQTT_Mgr[MQTT Manager] <--> MQTTBroker[MQTT Broker]
    WebServer[Web Server] <--> Clients[Web Clients]
    RelayCtrl[Relay Control] <--> SensorHub[SensorHub API]
    
    %% Time Synchronization Section
    Internet <--> NTPServer[NTP Server]
    NTPServer --> Timer
    
    %% Connections between sections
    MQTT_P --> MQTT_Mgr
    GState --> RelayCtrl
    WiFiMgr --> GState
```

## FreeRTOS Task Architecture
```mermaid
flowchart TD
    %% System Core
    SETUP[Setup and Initialization] --> RTOS[FreeRTOS Scheduler]
    LOOP[Main Loop] --> WDOG[Watchdog Feeding]
    LOOP --> NTPS[NTP Synchronization]
    LOOP --> MQTT_D[MQTT Discovery]
    LOOP --> NET_MON[Network Monitoring]
    LOOP --> MEM_CHK[Memory Monitoring]
    
    %% FreeRTOS Tasks
    RTOS --> DispTask[Display Task - Core 1]
    RTOS --> SensorTask[Sensor Task - Core 0]
    RTOS --> NetworkTask[Network Task - Core 0]
    
    %% Display Task Details
    DispTask --> DisplayQ[Display Queue]
    DisplayQ --> DispHandler[Display Handler]
    DispHandler --> DispBuffer[Display Buffer]
    DispBuffer --> SevenSeg[7-Segment Driver]
    
    %% Sensor Task Details
    SensorTask --> ReadBME[Read BME280]
    ReadBME --> GlobalS[Global State]
    ReadBME --> MQTT_Pub[MQTT Publishing]
    
    %% Network Task Details
    NetworkTask --> WatchdogF[Watchdog Feed]
    
    %% Inter-Task Communication
    GlobalS --> DispHandler
    MQTT_Pub --> MQTT_Cl[MQTT Client]
    WebS[Web Server] --> RelayC[Relay Controller]
    
    %% System Resources
    Mutex1[Display Mutex] --- DispHandler
    Mutex2[Network Mutex] --- MQTT_Cl
    Mutex3[Preferences Mutex] --- GlobalS
    SemQ1[Display Queue] --- DispTask
    SemQ2[Sensor Queue] --- SensorTask
```

## Device Startup and Communication Sequence
```mermaid
sequenceDiagram
 participant ESP as ESP32 Device
 participant WiFi as WiFi Network
 participant NTP as NTP Server
 participant MQTT as MQTT Broker
 participant BME as BME280 Sensor
 participant Display as 7-Segment Display
 participant Web as Web Browser
 
 ESP->>Display: Initialize (show device ID)
 ESP->>BME: Initialize I2C communication
 ESP->>ESP: Initialize PreferencesManager
 
 ESP->>WiFi: Check for stored credentials
 alt Has stored credentials
   ESP->>WiFi: Connect using credentials
   WiFi-->>ESP: Connection established
   
   ESP->>ESP: Set hostname to MQTT_CLIENT_ID
   ESP->>ESP: Setup mDNS as ablutionoracle3.local
   
   ESP->>NTP: Request time sync
   NTP-->>ESP: Receive current time
   ESP->>Display: Show current time
   
   ESP->>MQTT: Initialize connection
   MQTT-->>ESP: Connection established
   ESP->>MQTT: Publish online status
   ESP->>MQTT: Publish HomeAssistant discovery
   
   ESP->>ESP: Start WebServer in normal mode
 else No stored credentials
   ESP->>ESP: Start WiFi Portal Mode
   ESP->>Display: Show "AP" (Access Point)
   
   Web->>ESP: Connect to AP & configure WiFi
   ESP->>ESP: Store credentials & restart
 end
 
 loop Continuous Operation
   ESP->>BME: Read sensor data
   BME-->>ESP: Return temperature, humidity, pressure
   ESP->>ESP: Update GlobalState
   ESP->>Display: Update display based on mode
   
   ESP->>MQTT: Publish sensor readings
   
   alt Every 20 min
     ESP->>NTP: Resynchronize time
     NTP-->>ESP: Updated time
   end
   
   alt Every 60 min
     ESP->>MQTT: Refresh HomeAssistant discovery
   end
   
   ESP->>ESP: Monitor memory & system health
 end
 
 opt User Web Interaction
   Web->>ESP: HTTP request to device
   ESP-->>Web: Serve configuration page
   Web->>ESP: Save preferences
   ESP->>ESP: Update display preferences
   ESP->>Display: Apply brightness changes
 end
``` 

## How It Works
- **Global State Management:** Captures system-wide configurations and sensor data, ensuring synchronized operations.
- **Display Module:** Uses a dedicated FreeRTOS task to continuously update a physical display with digital clock and sensor data.
- **Sensor Reading:** The BME280 sensor is polled at intervals, and validated data (e.g., temperature, humidity, pressure) triggers MQTT publishing.
- **MQTT Connectivity:** The MQTTManager handles secure connections, reconnection strategies, and message publication with Last Will and Testament (LWT) for robust error handling.

## Challenges in Designing This Program & Library Choices

Developing this program came with some unexpected challenges. Initially, I used libraries I was already familiar with, along with code snippets I had happily reused from earlier projects. Everything was working smoothly—until I upgraded to a new Mac with Apple Silicon. Suddenly, some of my go-to libraries stopped working, painfully exposing the fact that they hadn’t been actively maintained for years.

Since I work on many different projects, I know that future me won’t always remember why I made certain choices. To help both myself and others who might be interested in this project, I’ve included a detailed description of the libraries used.

## What is ready and what is still left tot do:

- **OTA Implementaion:** Not yet implemented is OTA updates, making field upgrades seamless.

## Library Choices: Why These?

Below, you’ll find a brief explanation of each library used in this project, along with the reasoning behind its selection. In some cases, the choice was dictated by compatibility—some of the more popular libraries weren’t supported on macOS with ARM architecture. Wherever that was the case, I’ve noted it.

• Arduino.h  
  - Serves as the fundamental core library for ESP32 projects. While many alternatives exist, Arduino.h is widely supported on macOS ARM via the ESP32 toolchain, ensuring compatibility and ease of development.

• SPIFFS  
  - Provides onboard flash filesystem support. We chose SPIFFS because it is stable on ESP32 and has a proven track record—alternatives like LittleFS have had compatibility issues on macOS ARM when using certain development toolchains.

• Wire  
  - Used for I2C communication, a standard for sensor interfacing. The Wire library is robust and is maintained as part of the Arduino core for ESP32, which works reliably on macOS ARM without the issues that some non-standard I2C libraries might face.

• WiFi  
  - Manages WiFi connections. The ESP32 WiFi library (built into the core package) is well-tested and optimized for the ESP32. It avoids potential pitfalls found in some third-party libraries that may not support macOS ARM builds as seamlessly.

• esp_task_wdt  
  - Implements the watchdog timer using ESP-IDF components. This library is chosen for its low-level integration with the ESP32 system, ensuring robust task monitoring and reliability on macOS ARM-based development systems.

• esp_system.h  
  - Provides various essential system-level functions for the ESP environment. This is a core part of the ESP-IDF and is used because it offers direct access to hardware features, and has ensured compatibility even when more popular alternatives might have issues on macOS ARM.

• time.h  
  - Manages time synchronization, such as for NTP operations. The standard time library is chosen since it is part of the underlying system libraries and works effectively across different architectures including macOS ARM.

• Custom Libraries (GlobalState, DisplayHandler, BME280Handler, MQTTManager)  
  - These are developed in-house to encapsulate application-specific functionality. Their modular design ensures that they integrate seamlessly with the ESP32 core libraries and avoid reliance on external libraries that might not have full support on macOS ARM architecture.

By carefully selecting these libraries, we ensured stability, performance, and compatibility—especially important since some of the more popular alternatives may face compatibility issues on macOS ARM systems.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your enhancements or bug fixes.

## License
This project is licensed under the BSD 3-Clause License (Modified), see LICENSE file in the root of this project folder.



# Happy coding!


