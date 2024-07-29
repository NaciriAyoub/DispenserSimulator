#include "CUSTOMPINS.h"
#include <Arduino.h>

// Store the start time in milliseconds
unsigned long startTime = millis();

// Initialize Pulsers, Hooks, and Relays
Pulser Pulsers[6];  // Array of 5 Pulser instances
Hook Hooks[6];      // Array of 5 Hook instances
Relay Relays[6];    // Array of 5 Relay instances

int whiteLed = A0;
int redLed = 0;
int greenLed = 1;
int blueLed = 2;
int bypassS = 3;

void initAllPins() {
  
  // Initialize each Relay object in the array
  Serial.println(getElapsedTime() + "Init, Initializing Relays...");
  Relays[1] = Relay(1, A1);  // relayNumber 1, pinNumber A1
  Relays[2] = Relay(2, A2);  // relayNumber 2, pinNumber A2
  Relays[3] = Relay(3, A3);  // relayNumber 3, pinNumber A3
  Relays[4] = Relay(4, A4);  // relayNumber 4, pinNumber A4
  Relays[5] = Relay(5, A5);  // relayNumber 5, pinNumber A5

  // Initialize each Pulser object in the array
  Serial.println(getElapsedTime() + "Init, Initializing Pulsers...");
  Pulsers[1] = Pulser(1, 4);  // pulserNumber 1, pinNumber 4
  Pulsers[2] = Pulser(2, 5);  // pulserNumber 2, pinNumber 5
  Pulsers[3] = Pulser(3, 6);  // pulserNumber 3, pinNumber 6
  Pulsers[4] = Pulser(4, 7);  // pulserNumber 4, pinNumber 7
  Pulsers[5] = Pulser(5, 8);  // pulserNumber 5, pinNumber 8

  // Initialize each Hook object in the array
  Serial.println(getElapsedTime() + "Init, Initializing Hooks...");
  Hooks[1] = Hook(1, 9);   // hookNumber 1, pinNumber 9
  Hooks[2] = Hook(2, 10);  // hookNumber 2, pinNumber 10
  Hooks[3] = Hook(3, 11);  // hookNumber 3, pinNumber 11
  Hooks[4] = Hook(4, 12);  // hookNumber 4, pinNumber 12
  Hooks[5] = Hook(5, 13);  // hookNumber 5, pinNumber 13

  // Turn off Hook LED (LOW)
  for (int i = 1; i < 6; i++) {
    Hooks[i].Write(LOW);
  }

  // Turn off Pulse-1 LED (HIGH) - 5_Hose Pulse input is an inverter, Low turns on the Pulse LED
  for (int i = 1; i < 6; i++) {
    Pulsers[i].Write(HIGH);
  }

  // Set LED pins as OUTPUT
  pinMode(whiteLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(bypassS, OUTPUT);  // Set bypass switch pin as OUTPUT

  digitalWrite(whiteLed, LOW);  // Turn off White LED (LOW)
  digitalWrite(redLed, LOW);    // Turn off Red LED (LOW)
  digitalWrite(greenLed, LOW);  // Turn off Green LED (LOW)
  digitalWrite(blueLed, LOW);   // Turn off Blue LED (LOW)
  digitalWrite(bypassS, HIGH);  // Turn off Bypass_1-5 (HIGH)
}

void readAllHooks() {
  for (int i = 1; i < 6; i++) {
    Hooks[i].Read();
  }
}

void readAllRelays() {
  for (int i = 1; i < 6; i++) {
    Relays[i].Read();
  }
}

// Function to stringify an array of hooks
String stringifyHooks() {
    StaticJsonDocument<800> doc;
    JsonObject root = doc.to<JsonObject>();
    JsonArray hooksArray = root.createNestedArray("Hooks");

    for (int i = 1; i < 6; i++) {
        JsonObject hookObj = hooksArray.createNestedObject();
        Hooks[i].serialize(hookObj);
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Function to stringify an array of relays
String stringifyRelays() {
    StaticJsonDocument<800> doc;
    JsonObject root = doc.to<JsonObject>();
    JsonArray relaysArray = root.createNestedArray("Relays");

    for (int i = 1; i < 6; i++) {
        JsonObject relayObj = relaysArray.createNestedObject();
        Relays[i].serialize(relayObj);
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}



// Function to stringify an array of pulsers
String stringifyPulsers() {
    StaticJsonDocument<800> doc;
    JsonObject root = doc.to<JsonObject>();
    JsonArray pulsersArray = root.createNestedArray("Pulsers");

    for (int i = 1; i < 6; i++) {
        JsonObject pulserObj = pulsersArray.createNestedObject();
        Pulsers[i].serialize(pulserObj);
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Function to stringify all components
String stringifyAllComponents() {
  StaticJsonDocument<2000> doc;

  JsonObject ledState = doc.createNestedObject("LEDs");
  ledState["whiteLed"] = digitalRead(A0);
  ledState["redLed"] = digitalRead(0);
  ledState["greenLed"] = digitalRead(1);
  ledState["blueLed"] = digitalRead(2);
  ledState["bypassS"] = digitalRead(3);

  JsonArray relaysArray = doc.createNestedArray("Relays");
  for (int i = 1; i < 6; i++) {
    JsonObject relayObj = relaysArray.createNestedObject();
    Relays[i].serialize(relayObj);
  }

  JsonArray pulsersArray = doc.createNestedArray("Pulsers");
  for (int i = 1; i < 6; i++) {
    JsonObject pulserObj = pulsersArray.createNestedObject();
    Pulsers[i].serialize(pulserObj);
  }

  JsonArray hooksArray = doc.createNestedArray("Hooks");
  for (int i = 1; i < 6; i++) {
    JsonObject hookObj = hooksArray.createNestedObject();
    Hooks[i].serialize(hookObj);
  }

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void logMessage(const String& logType, String clientIp, const String& message) {
  String logMsg = logType + " ClientIP[" + clientIp + "]: " + message;
  Serial.println(logMsg);
  // Optionally, broadcast to all WebSocket clients
  wss.broadcast(WebSocket::DataType::TEXT, logMsg.c_str(), logMsg.length());
}

// Function to get the elapsed time since startTime as a String
String getElapsedTime() {
  // Calculate the elapsed time
  unsigned long now = millis();
  unsigned long elapsed = now - startTime;

  // Break down the elapsed time into hours, minutes, seconds, and milliseconds
  unsigned long hours = elapsed / 3600000;
  elapsed %= 3600000;
  unsigned long minutes = elapsed / 60000;
  elapsed %= 60000;
  unsigned long seconds = elapsed / 1000;
  unsigned long millis = elapsed % 1000;

  // Create the elapsed time string
  String elapsedTime = "[";
  if(hours > 0){
    elapsedTime += String(hours) + "h ";
  }
  if(minutes > 0){
    elapsedTime += String(minutes) + "m ";
  }
  if(seconds > 0){
    elapsedTime += String(seconds) + "s ";
  }
  if(millis > 0){
    elapsedTime += String(millis) + "ms";
  }
  elapsedTime +="] ";
  return elapsedTime;
}
