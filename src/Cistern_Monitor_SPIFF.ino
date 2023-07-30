/*
TODO:
 - fix low water float switch shorting to ground
 - remove src\Cistern_Monitor_SPIFF.ino; erase all git hub history for src\Cistern_Monitor_SPIFF.ino to remove wifi credentials
    - https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository
    - rename to open-cistern
 - toggle backlight / adjust backlight brightness via html button
 - add timestamp to high/low water alarm
 - test lost network connection js listener and UI
 - js notification API
 - sleep mode (wake on float trigger)
 - LCD display:
    - turn off backlight after timeout
    - turn on backlight if any global variable changes
    - pad ON/OFF text to 3 characters
 - move LCD code to lcd library directory
 - clean up css; add bootstrap responsive grid
RESEARCH/IDEAS:
 - use openthings to iframe app for remote
 - curl post to stats service
*/

// Import required libraries
//  #include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>

const int networkLedPin = 2;

const int lcdBrightnessPin = 15;

const int trigPin = 17; // trigger pin for JSN-SR04T
const int echoPin = 16; // echo pin for JSN-SR04T

const int lowLedPin = 12;
const int highLedPin = 13;

const int highFloatSwitch = 23;
const int lowFloatSwitch = 5;

bool highWaterAlarmState = 0;
bool lowWaterAlarmState = 0;

unsigned long lcdDelayTime = 10000; // lcd: show ssid and ip for 10 seconds; after 10 seconds show low/high alarm states and water level
unsigned long lcdBacklightOffTime = 600000;  // turn off lcd backlight after 10 minutes

String highWaterLcdAlarmText = "OFF";
String lowWaterLcdAlarmText = "OFF";

int prevDistance = 0;
int minWaterLevel = 0;
int maxWaterLevel = 0;

// Create MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const String mqttHost = "10.0.0.4";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

LiquidCrystal_I2C lcd(0x27, 16, 2);

void webSocketClientInit()
{
    JSONVar waterLevel;

    waterLevel["distance"] = prevDistance;
    waterLevel["minWaterLevel"] = minWaterLevel;
    waterLevel["maxWaterLevel"] = maxWaterLevel;

    notifyClients("WATER_LEVEL", waterLevel);
    notifyClients("HIGH_WATER_ALARM", false);
    notifyClients("LOW_WATER_ALARM", false);
}

void notifyClients(const String serverEventType, const JSONVar serverEventValue)
{
    JSONVar serverEvent;

    serverEvent["type"] = serverEventType;
    serverEvent["value"] = serverEventValue;

    String jsonString = JSON.stringify(serverEvent);

    ws.textAll(jsonString);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{

    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {

        JSONVar wsEvent = JSON.parse((char *)data);

        if (JSON.typeof(wsEvent) == "undefined")
        {
            Serial.println("wsEvent JSON Parsing input failed!");
            return;
        }

        if (wsEvent.hasOwnProperty("eventType"))
        {
            if (strcmp((const char *)wsEvent["eventType"], "init") == 0)
            {
                webSocketClientInit();
                return;
            }
            if (strcmp((const char *)wsEvent["eventType"], "reset") == 0)
            {
                // TODO: handle pump reset button
                // lowWaterAlarmState = 0;
                // notifyClients("RESET", true);

                prevDistance = 0;
                minWaterLevel = 0;
                maxWaterLevel = 0;

                return;
            }
            if (strcmp((const char *)wsEvent["eventType"], "status-message") == 0)
            {
                lcd.display();
                lcd.clear();
                // Print a message to the LCD.
                lcd.print((const char *)wsEvent["statusMessage"]);

                delay(3000);
                lcd.noDisplay();
                return;
            }
        }
        else
        {
            Serial.println("JSON parse error");
        }
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

void initMqtt()
{
    // connect to mqtt server
    mqttClient.setServer(mqttHost.c_str(), 1883);

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str())) {
           Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    String mqttMessage = "system-boot";
    mqttClient.publish("cistern-monitor", mqttMessage.c_str());

}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);


    lcd.init();
    lcd.backlight();

    analogWrite(lcdBrightnessPin, 125);

    // Clears the LCD screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting ...");

    if (!SPIFFS.begin(true))
    {
        Serial.println("ERROR: error while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");

    // open data/wifi_credentials.json for reading
    File file = SPIFFS.open("/wifi_credentials.json", FILE_READ);

    if (!file)
    {
        Serial.println("ERROR: problem opening data/wifi_credentials.json for reading");
        return;
    }

    String fileContent;
    while (file.available())
    {
        // Read line by line
        String line = file.readStringUntil('\n');
        fileContent += line; // Append each line to fileContent
    }

    file.close();

    JSONVar jsonCredentials = JSON.parse(fileContent);

    if (JSON.typeof(jsonCredentials) == "undefined")
    {
        Serial.println("ERROR: data/wifi_credentials.json error parsing JSON");
        return;
    }

    String password;
    if (jsonCredentials.hasOwnProperty("password"))
    {
        password = (const char *)jsonCredentials["password"];
    }

    String ssid;
    if (jsonCredentials.hasOwnProperty("ssid"))
    {
        ssid = (const char *)jsonCredentials["ssid"];
    }

    if (password.isEmpty() || ssid.isEmpty())
    {
        Serial.println("ERROR: password or ssid is emtpy.  please check the data/wifi_credentials.json file!");
        return;
    }

    pinMode(networkLedPin, OUTPUT);

    pinMode(lowLedPin, OUTPUT);
    pinMode(highLedPin, OUTPUT);

    digitalWrite(lowLedPin, LOW);
    digitalWrite(highLedPin, LOW);

    pinMode(lowFloatSwitch, INPUT_PULLUP);
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

    // Print ssid and ip address on LCD
    lcd.setCursor(0, 0);
    lcd.print(ssid);
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    lcd.setCursor(0, 0);

    initWebSocket();

    initMqtt();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(networkLedPin, HIGH);

        // reset backlight off timer
        lcdBacklightOffTime = millis() + 600000;
        lcd.backlight();
        analogWrite(lcdBrightnessPin, 125);

        // request->send(SPIFFS, "/index.html", "text/html", false, templateProcessor);
        request->send(SPIFFS, "/index.html", "text/html", false);

        delay(500);
        digitalWrite(networkLedPin, LOW);
    }
    );

    server.serveStatic("/", SPIFFS, "/");

    // Start ElegantOTA
    AsyncElegantOTA.begin(&server);

    // Start server
    server.begin();

    Serial.println("server started");
}

void loop()
{
    mqttClient.loop();

    ws.cleanupClients();

    digitalWrite(lowLedPin, lowWaterAlarmState); // low water float == 1 when above water line
    digitalWrite(highLedPin, highWaterAlarmState);

    // skip notifications until low water alarm has been reset
    // if (lowWaterAlarmState == 1)
    //     return;

    // TODO: if highWaterAlarmState == 1 alert browser max five times

    // ==  HIGH WATER FLOAT SENSOR == //
    int tempHighWaterAlarmState = digitalRead(highFloatSwitch);

    if (highWaterAlarmState != tempHighWaterAlarmState)
    {
        highWaterAlarmState = tempHighWaterAlarmState;
        digitalWrite(highLedPin, highWaterAlarmState);
        if (highWaterAlarmState == HIGH)
        {
            highWaterLcdAlarmText = "ON";
            notifyClients("HIGH_WATER_ALARM", true);
            Serial.println("HIGH_WATER_ALARM ON");
            mqttClient.publish("cistern-monitor/high-water-alarm", "on");
        }
        else
        {
            highWaterLcdAlarmText = "OFF";
            notifyClients("HIGH_WATER_ALARM", false);
            Serial.println("HIGH_WATER_ALARM OFF");
            mqttClient.publish("cistern-monitor/high-water-alarm", "off");
        }
    }

    // ==  LOW WATER FLOAT SENSOR == //
    // int tempLowWaterAlarmState = digitalRead(lowFloatSwitch);
    int tempLowWaterAlarmState = 0;

    if (lowWaterAlarmState != tempLowWaterAlarmState)
    {
        lowWaterAlarmState = tempLowWaterAlarmState;
        digitalWrite(lowLedPin, lowWaterAlarmState);
        if (lowWaterAlarmState == LOW)
        {
            lowWaterLcdAlarmText = "ON";

            // TODO: disable OpenSrpinkler
            // send curl request to http://10.0.0.152/cv?pw=4b5a7c40078b04f5c79c5f1a463141f3&en=0&_=1688566304970
            notifyClients("LOW_WATER_ALARM", true);
            Serial.println("LOW_WATER_ALARM ON");
            mqttClient.publish("cistern-monitor/low-water-alarm", "on");
        }
        else
        {
            lowWaterLcdAlarmText = "OFF";

            notifyClients("LOW_WATER_ALARM", false);
            Serial.println("LOW_WATER_ALARM OFF");
            mqttClient.publish("cistern-monitor/low-water-alarm", "off");
        }
    }

    // ==  ULTRASONIC SENSOR == //
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

    if (minWaterLevel == 0 || minWaterLevel > distance) minWaterLevel = distance;
    if (maxWaterLevel < distance) maxWaterLevel = distance;


    unsigned long currentTime = millis();

    if (currentTime >= lcdDelayTime)
    {
        if (distance != prevDistance)
        {
            prevDistance = distance;

            JSONVar waterLevel;

            waterLevel["distance"] = distance;
            waterLevel["minWaterLevel"] = minWaterLevel;
            waterLevel["maxWaterLevel"] = maxWaterLevel;

            notifyClients("WATER_LEVEL", waterLevel);
            mqttClient.publish("cistern-monitor/water-level", String(distance).c_str());

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.print("High:");
            lcd.print(highWaterLcdAlarmText);
            lcd.print(" Low:");
            lcd.print(lowWaterLcdAlarmText);
            lcd.setCursor(0, 1);
            lcd.printf("Water Level:%*d", 4, distance);
        }
    }

    if (currentTime >= lcdBacklightOffTime)
    {
        // turn off backlight after 10 minutes of inactivity
        lcd.noBacklight();
        analogWrite(lcdBrightnessPin, 0);
    }

    // One second delay before repeating measurement
    delay(1000);



}