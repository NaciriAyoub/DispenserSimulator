#ifndef CUSTOMPINS_H
#define CUSTOMPINS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketServer.h>

using namespace net;

// Forward declaration of WebSocket server
extern WebSocketServer wss;


struct Config {
  char SSID[25];          // Adjusted size to hold "DispenserSimulator_" + 4 characters + null terminator
  char wifiPassword[10];  // 9 characters + null terminator
  int majorVersion;
  int minorVersion;
  char guid[17];  // 16 characters + null terminator
};

struct ClientInfo {
  int id;
  String ip;
  WebSocket* socket;
  WiFiClient wifiClient;
};

// Logging function declaration
void logMessage(const String& logType, String clientIp, const String& message);
// Function to get the elapsed time since startTime as a String
String getElapsedTime();

class Hook {
public:
  int hookNumber;
  int pinNumber;
  bool state;

  Hook() {}
  Hook(int hookN, int pin) {
    hookNumber = hookN;
    pinNumber = pin;
    pinMode(pinNumber, OUTPUT);
    state = digitalRead(pinNumber);
    String msg = getElapsedTime() + "Init => " + GetHookString() + " Pin[" + String(pinNumber) + "]";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  bool Read() {
    PinStatus newState = digitalRead(pinNumber);
    if (state != newState) {
      state = newState;
      String msg = getElapsedTime() + GetHookString();
      Serial.println(msg);
      wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
    }
    return newState;
  }

  bool Write(bool newState) {
    if (newState != state) {
      digitalWrite(pinNumber, newState);
      state = newState;
      String msg = getElapsedTime() + GetHookString();
      Serial.println(msg);
      wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
    }
    return newState;
  }

  String GetHookString() {
    String hookS = state ? " OffHook" : " OnHook";
    return "Hook, " + String(hookNumber) + hookS;
  }

  String GetState() {
    return state ? "OffHook" : "OnHook";
  }

  PinStatus GetOppositeState() {
    return state ? LOW : HIGH;
  }

  void serialize(JsonObject& obj) {
    obj["Hook"] = hookNumber;
    obj["PinN"] = pinNumber;
    obj["State"] = GetState();
  }
};

class Relay {
public:
  int relayNumber;
  int pinNumber;
  bool state;

  Relay() {}
  Relay(int relayN, int pin) {
    relayNumber = relayN;
    pinNumber = pin;
    pinMode(pinNumber, INPUT);
    state = digitalRead(pinNumber);
    String msg = getElapsedTime() + "Init => " + GetRelayString() + " Pin[" + String(pinNumber) + "]";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  bool Read() {
    PinStatus newState = digitalRead(pinNumber);
    if (state != newState) {
      state = newState;
      String msg = getElapsedTime() + GetRelayString();
      Serial.println(msg);
      wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
    }
    return newState;
  }

  bool Write(PinStatus newState) {
    if (newState != state) {
      digitalWrite(pinNumber, newState);
      state = newState;
      String msg = getElapsedTime() + GetRelayString();
      Serial.println(msg);
      wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
    }
    return newState;
  }

  String GetRelayString() {
    String relayS = state ? " Closed" : " Opened";
    return "Relay, " + String(relayNumber) + relayS;
  }

  String GetState() {
    return state ? "Closed" : "Opened";
  }

  void serialize(JsonObject& obj) {
    obj["Relay"] = relayNumber;
    obj["PinN"] = pinNumber;
    obj["State"] = GetState();
  }
};

class Pulser {
public:
  int pulserNumber;
  int pinNumber;
  bool state = false;
  int pulsesRequested = 0;
  int delayBetweenPulses = 50;
  int pulsesRemaining = 0;
  bool pulsingActive = false;
  unsigned long previousMillis = 0;

  Pulser() = default;

  Pulser(int pulserN, int pin) {
    pulserNumber = pulserN;
    pinNumber = pin;
    pinMode(pinNumber, OUTPUT);
    state = digitalRead(pinNumber);
    String msg = getElapsedTime() + "Init => Pulser " + String(pulserNumber) + " Pin[" + String(pinNumber) + "]";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void SetPulses(int pulseCount) {
    pulsesRequested = pulseCount;
    pulsesRemaining = pulseCount;
    String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " Count " + String(pulseCount);
    Serial.println(msg);
    //wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void SetDelayBetweenPulses(int delayB) {
    delayBetweenPulses = delayB;
    String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " DelayB " + String(delayB) + " ms";
    Serial.println(msg);
    //wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void Update(unsigned long currentMillis) {
    if (pulsingActive && (currentMillis - previousMillis >= delayBetweenPulses)) {
      previousMillis = currentMillis;
      HandlePulsing();
    }
  }

  void StartPulsing(String clientIp) {
    if (pulsesRequested > 0 && !pulsingActive) {
      pulsingActive = true;
      previousMillis = millis();
      String msg = "Pulser " + String(pulserNumber) + " started pulsing. Count: " + String(pulsesRequested) + ", Delay: " + String(delayBetweenPulses) + " ms.";
      logMessage(getElapsedTime() + "Info", clientIp, msg);
    }
  }

  void StopPulsing() {
    pulsingActive = false;
    String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " stopped pulsing.";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void EndPulsing() {
    pulsingActive = false;
    String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " ended pulsing.";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void ContinuePulsing(String clientIp) {
    if (pulsesRemaining > 0 && !pulsingActive) {
      pulsingActive = true;
      previousMillis = millis();
      String msg = "Pulser " + String(pulserNumber) + " continued pulsing. Remaining: " + String(pulsesRemaining) + ", Delay: " + String(delayBetweenPulses) + " ms.";
      logMessage(getElapsedTime() + "Info", clientIp, msg);
    }
  }

  void CancelPulsing() {
    pulsingActive = false;
    pulsesRemaining = 0;
    String msg = getElapsedTime() + "Pulser " + String(pulserNumber) + " canceled pulsing.";
    Serial.println(msg);
    wss.broadcast(WebSocket::DataType::TEXT, msg.c_str(), msg.length());
  }

  void HandlePulsing() {
    if (pulsesRemaining > 0) {
      digitalWrite(pinNumber, LOW);
      delay(5);
      digitalWrite(pinNumber, HIGH);
      pulsesRemaining--;
      //Serial.println(getElapsedTime() + "Pulser, " + String(pulserNumber) + " " + String(pulsesRemaining));
    } else {
      EndPulsing();
    }
  }

  bool Write(PinStatus newState) {
    if (newState != state) {
      digitalWrite(pinNumber, newState);
      state = newState;
    }
    return newState;
  }

  String GetState() {
    return pulsingActive ? "Pulsing" : "Idle";
  }

  void serialize(JsonObject& obj) {
    obj["Pulser"] = pulserNumber;
    obj["PinN"] = pinNumber;
    obj["PinN"] = pinNumber;
    obj["PulsesRequested"] = pulsesRequested;
    obj["DelayBetweenPulses"] = delayBetweenPulses;
    obj["PulsesRemaining"] = pulsesRemaining;
    obj["State"] = GetState();
  }
};

// Global variables
extern unsigned long startTime;
extern Pulser Pulsers[];
extern Hook Hooks[];
extern Relay Relays[];

extern int whiteLed;
extern int redLed;
extern int greenLed;
extern int blueLed;
extern int bypassS;

void initAllPins();
void readAllHooks();
void readAllRelays();
String stringifyHooks();
String stringifyRelays();
String stringifyPulsers();
String stringifyAllComponents();

#endif
