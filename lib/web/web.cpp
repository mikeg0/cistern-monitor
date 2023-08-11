#include "../constants.h"

#include "web.h"
#include <LiquidCrystal_I2C.h>
#include <AsyncElegantOTA.h>
#include <SPIFFS.h>
#include "alarm_sound.h"

extern LiquidCrystal_I2C lcd;


extern int currentWaterLevel;
extern int minWaterLevel;
extern int maxWaterLevel;
extern int realTimeStats;
extern int highWaterAlarmState;
extern int lowWaterAlarmState;

// extern const int networkLedPin;
// extern const int lcdBrightnessPin;
// extern const int lowLedPin;
// extern const int highLedPin;



// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void CMWeb::notifyClients(const String serverEventType, const JSONVar serverEventValue)
{
    JSONVar serverEvent;

    serverEvent["type"] = serverEventType;
    serverEvent["value"] = serverEventValue;

    String jsonString = JSON.stringify(serverEvent);

    ws.textAll(jsonString);
}


void CMWeb::_webSocketClientInit()
{
    JSONVar waterLevel;

    waterLevel["currentWaterLevel"] = currentWaterLevel;
    waterLevel["minWaterLevel"] = minWaterLevel;
    waterLevel["maxWaterLevel"] = maxWaterLevel;

    CMWeb::notifyClients("WATER_LEVEL", waterLevel);
    CMWeb::notifyClients("HIGH_WATER_ALARM", (highWaterAlarmState) ? true : false);
    CMWeb::notifyClients("LOW_WATER_ALARM", (lowWaterAlarmState) ? true : false);
    CMWeb::notifyClients("REAL_TIME_STATS", realTimeStats);
}

void CMWeb::_handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
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
                CMWeb::_webSocketClientInit();
                return;
            }

            if (strcmp((const char *)wsEvent["eventType"], "reset-high-water-alarm") == 0)
            {
                // turn off low water alarm LED (but keep LCD and web alarm UI on)
                digitalWrite(highLedPin, 0);

                // TODO: turn off high water alarm speaker

                return;
            }

            if (strcmp((const char *)wsEvent["eventType"], "reset-low-water-alarm") == 0)
            {
                // turn off low water alarm LED (but keep LCD and web alarm UI on)
                digitalWrite(lowLedPin, 0);

                // TODO: send curl request message to Open Sprinkler to re-enable
                return;
            }


            // temporary setting to toggle real-time water level status to websocket
            if (strcmp((const char *)wsEvent["eventType"], "real-time") == 0)
            {
                realTimeStats = (int)wsEvent["value"];
                notifyClients("REAL_TIME_STATS", realTimeStats);

                if (realTimeStats == 0) {
                    lcd.noBacklight();
                    analogWrite(lcdBrightnessPin, 0);
                }
                else {
                    lcd.backlight();
                    analogWrite(lcdBrightnessPin, 125);
                }


                return;
            }

            if (strcmp((const char *)wsEvent["eventType"], "status-message") == 0)
            {
                lcd.display();
                lcd.clear();
                // Print a message to the LCD.
                lcd.print((const char *)wsEvent["statusMessage"]);

                alarmSound();

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

void CMWeb::_onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
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
        _handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void CMWeb::cleanupClients()
{
    ws.cleanupClients();
}

void CMWeb::initWebSocket()
{
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        digitalWrite(networkLedPin, HIGH);

        request->send(SPIFFS, "/index.html", "text/html", false);

        delay(500);
        digitalWrite(networkLedPin, LOW);
    });

    server.serveStatic("/", SPIFFS, "/");

    // Start ElegantOTA
    AsyncElegantOTA.begin(&server);

    // Start server
    server.begin();

    Serial.println("server started");


    ws.onEvent(CMWeb::_onEvent);
    server.addHandler(&ws);
}
