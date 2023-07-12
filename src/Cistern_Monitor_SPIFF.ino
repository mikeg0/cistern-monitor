

/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

/*
TODO:
 - high/low water level to global var
 - reset high/low water level var from html button
 - lost network connection js listener and UI
 - js notification API
 - OTA firmware updates
 - sleep mode (wake on HTTP request)

*/

// Import required libraries
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"

// Replace with your network credentials
const char *ssid = "ConexED_2GEXT";
const char *password = "Bubbagirl";

const int networkLedPin = 2;

const int trigPin = 17; // trigger pin for JSN-SR04T
const int echoPin = 16; // echo pin for JSN-SR04T

const int lowLedPin = 12;
const int highLedPin = 13;

const int highFloatSwitch = 23;
const int lowFloatSwitch = 22;

bool highWaterAlarmState = 0;
bool lowWaterAlarmState = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients(const String &serverEventType, const int value)
{
    JSONVar serverEvent;

    serverEvent["type"] = serverEventType;
    serverEvent["value"] = value;

    String jsonString = JSON.stringify(serverEvent);


    ws.textAll(jsonString);

/*
    size_t len = measureJson(root);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
    if (buffer)
    {
        serializeJson(root, (char *)buffer->get(), len + 1);
        ws.textAll(buffer);
    }
*/
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {

        data[len] = 0;
        if (strcmp((char *)data, "reset") == 0)
        {
            lowWaterAlarmState = 0;
            notifyClients("RESET", 0);
        }

        // TODO: handle pump reset button
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

String templateProcessor(const String &var)
{
    Serial.println(var);
    if (var == "LOW_WATER_ALARM")
    {
        if (lowWaterAlarmState)
        {
            return "ON";
        }
        else
        {
            return "OFF";
        }
    }

    if (var == "HIGH_WATER_ALARM")
    {
        if (highWaterAlarmState)
        {
            return "ON";
        }
        else
        {
            return "OFF";
        }
    }

    return String();
}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

    pinMode(networkLedPin, OUTPUT);

    //  pinMode(lowLedPin, OUTPUT);
    pinMode(highLedPin, OUTPUT);

    //  digitalWrite(lowLedPin, LOW);
    digitalWrite(highLedPin, LOW);

    //  pinMode(lowFloatSwitch, INPUT_PULLUP);
    pinMode(highFloatSwitch, INPUT_PULLUP);

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP Local IP Address
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (!SPIFFS.begin(true))
    {
        Serial.println("An error has occurred while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");

    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

        digitalWrite(networkLedPin, HIGH);

        request->send(SPIFFS, "/index.html", "text/html", false, templateProcessor);

        delay(500);
        digitalWrite(networkLedPin, LOW);

    });

    server.serveStatic("/", SPIFFS, "/");

    // Start ElegantOTA
    AsyncElegantOTA.begin(&server);

    // Start server
    server.begin();

    Serial.println("server started");
}

void loop()
{
    ws.cleanupClients();

    //  digitalWrite(lowLedPin, lowWaterAlarmState);
    digitalWrite(highLedPin, highWaterAlarmState);

    // skip notifications until low water alarm has been reset
    if (lowWaterAlarmState == 1)
        return;

    // TODO: if highWaterAlarmState == 1 alert browser max five times

    int tempHighWaterAlarmState = digitalRead(highFloatSwitch);

    if (highWaterAlarmState != tempHighWaterAlarmState)
    {
        highWaterAlarmState = tempHighWaterAlarmState;
        digitalWrite(highLedPin, highWaterAlarmState);
        if (highWaterAlarmState == HIGH)
        {
            notifyClients("HIGH_WATER_ALARM", 1);
            Serial.println("HIGH_WATER_ALARM ON");
        }
        else
        {
            notifyClients("HIGH_WATER_ALARM", 0);
            Serial.println("HIGH_WATER_ALARM OFF");
        }
    }

    /*
    tempLowWaterAlarmState = digitalRead(lowFloatSwitch);

    if (lowWaterAlarmState != tempLowWaterAlarmState) {
      lowWaterAlarmState = tempLowWaterAlarmState;
      if (lowWaterAlarmState == LOW) {
        // TODO: disable OpenSrpinkler
        // send curl request to http://10.0.0.152/cv?pw=4b5a7c40078b04f5c79c5f1a463141f3&en=0&_=1688566304970
        notifyClients("LOW_WATER_ALARM", 1);
        Serial.println("LOW_WATER_ALARM ON");
      } else {
        notifyClients("LOW_WATER_ALARM", 0);
        Serial.println("LOW_WATER_ALARM OFF");
      }
    }
    */

    // Set the trigger pin LOW for 2uS
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // Set the trigger pin HIGH for 20us to send pulse
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(20);

    // Return the trigger pin to LOW
    digitalWrite(trigPin, LOW);

    // Measure the width of the incoming pulse
    int duration = pulseIn(echoPin, HIGH);

    // Determine distance from duration
    // Use 343 metres per second as speed of sound
    // Divide by 1000 as we want millimeters

    int distance = (duration / 2) * 0.343;

    // Print result to serial monitor
    Serial.print(" distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    notifyClients("WATER_LEVEL", distance);

    // Delay before repeating measurement

    delay(1000);
}