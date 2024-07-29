#include <EEPROM.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <WebSocketServer.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"
#include "CUSTOMPINS.h"
#include "index.h"

using namespace net;

// Other global variables...
bool ready = false;
int interval = 10000;              // every 10000ms -> 10s
unsigned long previousMillis = 0;  // we use the "millis()" command for time reference and this will output an unsigned long
int wifiStatus = WL_IDLE_STATUS;
ArduinoLEDMatrix matrix;

const int CONFIG_START_ADDRESS = 0;   // Starting address for the configuration
const int CONFIG_FLAG_ADDRESS = 100;  // Address to store the configuration flag
const int CHECKSUM_ADDRESS = 101;     // Address to store the checksum
Config config;

WebSocketServer wss(81);
WiFiServer server(80);



// Function declarations
void checkConfig();
void setWifi();
void testRandomHookSignals();
void checkWifiClients(WiFiClient client);
void onWebSocketMessage(WebSocket& ws, WebSocket::DataType dataType, const char* message, uint16_t length);
void onWebSocketClose(WebSocket& ws, WebSocket::CloseCode code, const char* reason, uint16_t length);
void checkWebSocketMessages(WebSocket& ws, const char* message, String clientIp);

void setup() {
  // Set Matrix
  matrix.begin();
  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.begin();
  matrix.play(true);

  // Set Serial
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("************************");
  Serial.println(getElapsedTime() + "Serial, Serial is ready!");

  // Initialize EEPROM
  Serial.println(getElapsedTime() + "EEPROM, Initializing...");
  EEPROM.begin();
  checkConfig();

  setWifi();
  server.begin();
  Serial.println(getElapsedTime() + "Wifi, WebServer Started.");

  setWebSockets();
  wss.begin();
  Serial.println(getElapsedTime() + "Wifi, WebSocketServer Started.");

  // Set Pins && Values
  initAllPins();
  Serial.println(getElapsedTime() + "Setup, Pins Init completed successfully.");

  ready = true;
}

void loop() {
  digitalWrite(redLed, HIGH);

  readAllRelays();
  readAllHooks();
  //testRandomHookSignals();

  wss.listen();
  WiFiClient client = server.available();
  checkWifiClients(client);

  // Update all pulsers
  unsigned long currentMillis = millis();
  for (int i = 1; i < 6; ++i) {
    Pulsers[i].Update(currentMillis);
  }

  delay(10);
  digitalWrite(redLed, LOW);
}

// Function to set up WebSockets
void setWebSockets() {
  Serial.println(getElapsedTime() + "Wifi, Setting up WebSocket server...");
  wss.onConnection([](WebSocket& ws) {
    String clientIp = ws.getRemoteIP().toString();
    logMessage(getElapsedTime() + "Info", clientIp, "New WebSocket Connection established.");

    ws.onMessage(onWebSocketMessage);
    ws.onClose(onWebSocketClose);

    String msg = "Hi Client IP[" + clientIp + "]" + " from Dispenser Simulator.";
    Serial.println(msg);
    ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  });
}

// Function to handle incoming WebSocket messages
void onWebSocketMessage(WebSocket& ws, WebSocket::DataType dataType, const char* message, uint16_t length) {
  String clientIp = ws.getRemoteIP().toString();
  switch (dataType) {
    case WebSocket::DataType::TEXT:
      logMessage(getElapsedTime() + "Info", clientIp, String("Received: ") + message);
      checkWebSocketMessages(ws, message, clientIp);
      break;
    case WebSocket::DataType::BINARY:
      logMessage(getElapsedTime() + "Info", clientIp, "Received binary data");
      break;
  }

  //String reply = "Received: " + String(message);
  //ws.send(dataType, reply.c_str(), reply.length());
}

// Function to handle WebSocket close event
void onWebSocketClose(WebSocket& ws, WebSocket::CloseCode code, const char* reason, uint16_t length) {
  String clientIp = ws.getRemoteIP().toString();
  logMessage(getElapsedTime() + "Info", clientIp, "Disconnected");
}

// Function to process WebSocket messages based on their content
void checkWebSocketMessages(WebSocket& ws, const char* message, String clientIp) {
  // Convert to Arduino String
  String strMessage = String(message);
  String logPrefix = "Client requested ";

  if (strMessage.startsWith("Pulser ")) {
    int pulserNumber = strMessage.substring(7, 8).toInt();
    String action = strMessage.substring(9);

    if (pulserNumber >= 1 && pulserNumber <= 5) {
      if (action == "Stop") {
        Pulsers[pulserNumber].StopPulsing();
        String msg = "Pulser " + String(pulserNumber) + " stopped.";
        logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
        ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
      } else if (action == "Continue") {
        Pulsers[pulserNumber].ContinuePulsing(clientIp);
        String msg = "Pulser " + String(pulserNumber) + " continued.";
        logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
        ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
      } else if (action == "Cancel") {
        Pulsers[pulserNumber].CancelPulsing();
        String msg = "Pulser " + String(pulserNumber) + " canceled.";
        logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
        ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
      } else {
        // Handle Pulsing command
        int countIndex = action.indexOf("Count");
        int delayBIndex = action.indexOf("DelayB");

        if (countIndex != -1 && delayBIndex != -1) {
          int count = action.substring(countIndex + 6, delayBIndex - 1).toInt();
          int delayB = action.substring(delayBIndex + 7).toInt();

          Pulsers[pulserNumber].SetPulses(count);  // Adjust array index
          Pulsers[pulserNumber].SetDelayBetweenPulses(delayB);
          Pulsers[pulserNumber].StartPulsing(clientIp);
          String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " set to pulse " + String(count) + " times with a delay of " + String(delayB) + " ms.";
          logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
          ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
        } else {
          String error = "Invalid pulser command format.";
          logMessage(getElapsedTime() + "Pulser Error", clientIp, logPrefix + error);
        }
      }
    } else {
      String error = "Invalid pulser number.";
      logMessage(getElapsedTime() + "Pulser Error", clientIp, logPrefix + error);
    }
  } else if (strMessage.startsWith("Hook ")) {
    int hookNumber = strMessage.substring(5, 6).toInt();
    if (strMessage.endsWith("OnHook")) {
      if (Hooks[hookNumber].GetState() == "OffHook") {
        Hooks[hookNumber].Write(LOW);
        String msg = "Hook " + String(hookNumber) + " OnHook";
        logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
        ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
      }
    } else if (strMessage.endsWith("OffHook")) {
      if (Hooks[hookNumber].GetState() == "OnHook") {
        Hooks[hookNumber].Write(HIGH);
        String msg = "Hook " + String(hookNumber) + " OffHook";
        logMessage(getElapsedTime() + "Info", clientIp, logPrefix + msg);
        ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
      }
    } else {
      String error = "Invalid hook command.";
      logMessage(getElapsedTime() + "Hook Error", clientIp, logPrefix + error);
    }
  } else if (strMessage == "Relays") {
    String msg = stringifyRelays();
    Serial.println(getElapsedTime() + "Wifi, Sending relays data: " + msg);
    ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  } else if (strMessage == "Hooks") {
    String msg = stringifyHooks();
    Serial.println(getElapsedTime() + "Wifi, Sending hooks data: " + msg);
    ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  } else if (strMessage == "Pulsers") {
    String msg = stringifyPulsers();
    Serial.println(getElapsedTime() + "Wifi, Sending pulsers data: " + msg);
    ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  } else if (strMessage == "All") {
    String msg = stringifyAllComponents();
    Serial.println(getElapsedTime() + "Wifi, Sending all components data: " + msg);
    ws.send(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  } else {
    Serial.println(getElapsedTime() + "Wifi, Received unknown WebSocket message: " + strMessage);
  }
}

// Function to check and load the configuration from EEPROM
void checkConfig() {
  if (!isConfigExists() || !isChecksumValid()) {
    // Create and save the configuration if it doesn't exist or checksum is invalid
    config = createDefaultConfig();
    if (!saveConfigToEEPROM(config)) {
      Serial.println(getElapsedTime() + "Config, Error saving config to EEPROM");
    }
  } else {
    // Read the configuration from EEPROM
    if (!readConfigFromEEPROM(config)) {
      Serial.println(getElapsedTime() + "Config, Error reading config from EEPROM");
    }
  }
}

// Function to generate random hook signals
void testRandomHookSignals() {
  unsigned long now = millis();
  if ((unsigned long)(now - previousMillis) > interval) {
    previousMillis = now;  // reset previousMillis
    int randomNumber = random(1, 6);
    PinStatus newstate = Hooks[randomNumber].GetOppositeState();
    Serial.println(getElapsedTime() + "- Testing Random Hook Signal: Hook[" + String(randomNumber) + "]");
    Hooks[randomNumber].Write(newstate);
  }
}

// Function to set up WiFi as an access point
void setWifi() {
  Serial.println(getElapsedTime() + "Wifi, Setting up Access Point ...");
  // Check if the WiFi module is present
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(getElapsedTime() + "Wifi, Communication with WiFi module failed!");
    while (true)
      ;
  }
  // Print firmware version
  String fv = WiFi.firmwareVersion();
  Serial.println(getElapsedTime() + "Wifi, Firmware Version = " + fv);

  // Convert the SSID to a char array
  char ssidChar[sizeof(config.SSID)];
  strcpy(ssidChar, config.SSID);

  // Set the Arduino as an access point
  wifiStatus = WiFi.beginAP(ssidChar, config.wifiPassword);

  Serial.println(String(getElapsedTime() + "Wifi, SSID = ") + ssidChar);
  Serial.println(String(getElapsedTime() + "Wifi, Password = ") + config.wifiPassword);
  Serial.print(getElapsedTime() + "Wifi, Local IP = ");
  Serial.println(WiFi.localIP());

  delay(500);
  matrix.loadSequence(LEDMATRIX_ANIMATION_ARROWS_COMPASS);

  // Check for successful AP setup
  if (wifiStatus != WL_AP_LISTENING) {
    Serial.println(getElapsedTime() + "Wifi, Creating AP failed.");
    while (true)
      ;
  }
  Serial.println(getElapsedTime() + "Wifi, AP set up successfully.");

  delay(500);
  matrix.loadSequence(LEDMATRIX_ANIMATION_SPINNING_COIN);
}

// Function to check for connected WiFi clients
void checkWifiClients(WiFiClient client) {
  if (client) {

    // Log client connection
    String clientIp = client.remoteIP().toString();
    logMessage(getElapsedTime() + "Info", clientIp, "New WiFi Client Connection.");

    // Read the HTTP request header line by line
    while (client.connected()) {
      if (client.available()) {
        String HTTP_header = client.readStringUntil('\n');  // read the header line of HTTP request

        if (HTTP_header.equals("\r"))  // the end of HTTP request
          break;

        Serial.print("Wifi, << ");
        Serial.println(HTTP_header);  // print HTTP request to Serial Monitor
      }
    }

    // Send the HTTP response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    client.println();                     // the separator between HTTP header and body

    client.println(htmlPage);
    client.flush();

    // Give the web browser time to receive the data
    delay(10);

    // Close the connection
    client.stop();
    Serial.println(getElapsedTime() + "Wifi, Client " + clientIp + " disconnected.");

    // Log client disconnection
    logMessage(getElapsedTime() + "Info", clientIp, "WiFi client handled and disconnected.");
  }
}

// Function to generate a random string of specified length
String generateRandomString(int length) {
  const char charset[] = "0123456789";
  String result = "";

  for (int i = 0; i < length; i++) {
    int index = random(0, sizeof(charset) - 1);
    result += charset[index];
  }

  return result;
}

// Function to check if configuration exists in EEPROM
bool isConfigExists() {
  // Read the configuration flag
  return EEPROM.read(CONFIG_FLAG_ADDRESS) == 1;
}

// Function to check if checksum is valid
bool isChecksumValid() {
  // Calculate the checksum for the current configuration in EEPROM
  Config config;
  if (!readConfigFromEEPROM(config)) {
    return false;
  }
  uint8_t storedChecksum = EEPROM.read(CHECKSUM_ADDRESS);
  return storedChecksum == calculateChecksum(config);
}

// Function to save configuration to EEPROM
bool saveConfigToEEPROM(const Config& config) {
  EEPROM.put(CONFIG_START_ADDRESS, config);  // Save the configuration
  uint8_t checksum = calculateChecksum(config);
  EEPROM.write(CHECKSUM_ADDRESS, checksum);  // Save the checksum
  EEPROM.write(CONFIG_FLAG_ADDRESS, 1);      // Set the configuration flag
  Serial.println(getElapsedTime() + "Config, Successfully saved config to EEPROM.");
  return true;  // Indicate success
}

// Function to read configuration from EEPROM
bool readConfigFromEEPROM(Config& config) {
  EEPROM.get(CONFIG_START_ADDRESS, config);  // Read the configuration
  // Verify the read operation
  if (EEPROM.length() < sizeof(config)) {
    Serial.println(getElapsedTime() + "Config, EEPROM read error: size mismatch.");
    return false;  // Indicate failure
  }
  return true;  // Indicate success
}

// Function to create default configuration
Config createDefaultConfig() {
  // Seed the random number generator
  randomSeed(analogRead(A0));
  Config config;
  // Generate the full SSID with the random 4-character string
  String randomPart = generateRandomString(4);
  String fullSSID = String("DispenserSimulator_") + randomPart;
  strncpy(config.SSID, fullSSID.c_str(), sizeof(config.SSID));
  strncpy(config.wifiPassword, "12345678", sizeof(config.wifiPassword));
  config.majorVersion = 1;
  config.minorVersion = 0;
  String uniqueGuid = generateRandomString(16);
  strncpy(config.guid, uniqueGuid.c_str(), sizeof(config.guid));
  Serial.println(getElapsedTime() + "Config, DefaultConfig, Wifi SSID generated: " + fullSSID);
  Serial.println(getElapsedTime() + "Config, DefaultConfig, Wifi Password: 12345678");
  Serial.println(getElapsedTime() + "Config, DefaultConfig, Dispenser Simulator SerialNumber: " + uniqueGuid);
  return config;
}

// Function to calculate checksum
uint8_t calculateChecksum(const Config& config) {
  const uint8_t* p = (const uint8_t*)&config;
  uint8_t checksum = 0;
  for (size_t i = 0; i < sizeof(config); ++i) {
    checksum ^= p[i];
  }
  return checksum;
}

// Function to print version of the configuration
void printVersion(const Config& config) {
  Serial.print(getElapsedTime() + "Config, Version: ");
  Serial.print(config.majorVersion);
  Serial.print(".");
  Serial.println(config.minorVersion);
}
