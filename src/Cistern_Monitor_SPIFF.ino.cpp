# 1 "C:\\Users\\micha\\AppData\\Local\\Temp\\tmpbnerncdx"
#include <Arduino.h>
# 1 "C:/dev/cistern-monitor/src/Cistern_Monitor_SPIFF.ino"
# 28 "C:/dev/cistern-monitor/src/Cistern_Monitor_SPIFF.ino"
#include "../lib/constants.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Arduino_JSON.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include "utils.h"
#include "alarm_sound.h"
#include "web.h"

int highWaterAlarmState = 0;
int lowWaterAlarmState = 0;

String highWaterLcdAlarmText = "OFF";
String lowWaterLcdAlarmText = "OFF";

int currentWaterLevel = 0;
int pumpCurrent = 0;
int minWaterLevel = 0;
int maxWaterLevel = 0;

int realTimeStats = 1;

float MIN_PS_VOLTAGE = 1.6;
float MAX_PS_VOLTAGE = 4.0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
CMWeb cisternMonitorWeb;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_ADS1115 ads;
void connectMqtt(int initialConnect);
void setup();
void loop();
#line 65 "C:/dev/cistern-monitor/src/Cistern_Monitor_SPIFF.ino"
void connectMqtt(int initialConnect)
{

    if (initialConnect) {

        mqttClient.setServer(mqttHost.c_str(), 1883);
    }

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");

        String clientId = "cistern-monitor-client";


        if (mqttClient.connect(clientId.c_str())) {
           Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");

            delay(5000);
        }
    }

    if (initialConnect) {
        String mqttMessage = "system-boot";
        mqttClient.publish("cistern-monitor", mqttMessage.c_str());
    }
    else {
        String mqttMessage = "system-reconnect";
        mqttClient.publish("cistern-monitor", mqttMessage.c_str());
    }

}

void setup()
{

    Serial.begin(115200);

    Wire.begin();
    ads.begin();
    lcd.begin(16, 2);

    lcd.backlight();

    analogWrite(lcdBrightnessPin, 125);


    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting ...");


    startupSound();

    if (!SPIFFS.begin(true))
    {
        Serial.println("ERROR: error while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");


    File file = SPIFFS.open("/wifi_credentials.json", FILE_READ);

    if (!file)
    {
        Serial.println("ERROR: problem opening data/wifi_credentials.json for reading");
        return;
    }

    String fileContent;
    while (file.available())
    {

        String line = file.readStringUntil('\n');
        fileContent += line;
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

    pinMode(lowFloatSwitch, INPUT);
    pinMode(highFloatSwitch, INPUT);


    IPAddress ip(10, 0, 0, 150);
    IPAddress gateway(10, 0, 0, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    WiFi.setHostname("Cistern Monitor");


    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    lcd.setCursor(0, 0);
    lcd.print(ssid);
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    lcd.setCursor(0, 0);

    cisternMonitorWeb.initWebSocket();

    connectMqtt(1);

}


void loop()
{

    if (!mqttClient.connected()) {
        connectMqtt(0);
    }
    mqttClient.loop();

    cisternMonitorWeb.cleanupClients();

    if (realTimeStats == 0) return;

    digitalWrite(lowLedPin, lowWaterAlarmState);
    digitalWrite(highLedPin, highWaterAlarmState);







    float ps_reading = get_pressure_sensor_reading(ads);
    float ct_reading = get_current_transformer_reading(ads);

    Serial.print("ct_reading: ");
    Serial.println(ct_reading);

    Serial.print("ps_reading: ");
    Serial.println(ps_reading);

    if (ps_reading > 0.01) {
        currentWaterLevel = ((ps_reading - MIN_PS_VOLTAGE) / (MAX_PS_VOLTAGE - MIN_PS_VOLTAGE)) * 100;
    }


    int tempHighWaterAlarmState = digitalRead(highFloatSwitch);

    if (tempHighWaterAlarmState == HIGH)
    {
        alarmSound();
    }

    if (highWaterAlarmState != tempHighWaterAlarmState)
    {
        highWaterAlarmState = tempHighWaterAlarmState;
        digitalWrite(highLedPin, highWaterAlarmState);
        if (highWaterAlarmState == HIGH)
        {
            highWaterLcdAlarmText = "ON";
            cisternMonitorWeb.notifyClients("HIGH_WATER_ALARM", true);
            Serial.println("HIGH_WATER_ALARM ON");
            mqttClient.publish("cistern-monitor/high-water-alarm", "on");
        }
        else
        {
            highWaterLcdAlarmText = "OFF";
            cisternMonitorWeb.notifyClients("HIGH_WATER_ALARM", false);
            Serial.println("HIGH_WATER_ALARM OFF");
            mqttClient.publish("cistern-monitor/high-water-alarm", "off");
        }
    }


    int tempLowWaterAlarmState = digitalRead(lowFloatSwitch);

    if (lowWaterAlarmState != tempLowWaterAlarmState)
    {
        lowWaterAlarmState = tempLowWaterAlarmState;
        digitalWrite(lowLedPin, lowWaterAlarmState);
        if (lowWaterAlarmState == HIGH)
        {
            lowWaterLcdAlarmText = "ON";



            cisternMonitorWeb.notifyClients("LOW_WATER_ALARM", true);
            Serial.println("LOW_WATER_ALARM ON");
            mqttClient.publish("cistern-monitor/low-water-alarm", "on");
        }
        else
        {
            lowWaterLcdAlarmText = "OFF";

            cisternMonitorWeb.notifyClients("LOW_WATER_ALARM", false);
            Serial.println("LOW_WATER_ALARM OFF");
            mqttClient.publish("cistern-monitor/low-water-alarm", "off");
        }
    }


    if (minWaterLevel == 0 || minWaterLevel > currentWaterLevel) minWaterLevel = currentWaterLevel;
    if (maxWaterLevel < currentWaterLevel) maxWaterLevel = currentWaterLevel;

    unsigned long currentTime = millis();





    JSONVar waterLevel;

    waterLevel["waterLevel"] = currentWaterLevel;
    waterLevel["minWaterLevel"] = minWaterLevel;
    waterLevel["maxWaterLevel"] = maxWaterLevel;

    float pumpCurrent = 0;

    if (ct_reading > 0.01) {
        pumpCurrent = ct_reading * 30;
    }

    waterLevel["pumpCurrent"] = String(pumpCurrent, 2);
    waterLevel["ctVoltage"] = String(ct_reading, 5);

    waterLevel["psVoltage"] = String(ps_reading, 5);

    cisternMonitorWeb.notifyClients("SYSTEM_STATE", waterLevel);



    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("High:");
    lcd.print(highWaterLcdAlarmText);
    lcd.print(" Low:");
    lcd.print(lowWaterLcdAlarmText);
    lcd.setCursor(0, 1);
    lcd.printf("W: %3d%%", (int)currentWaterLevel);
    lcd.printf(" P: %s", String(pumpCurrent, 2));


    delay(1000);

}