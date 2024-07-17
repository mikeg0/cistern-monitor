#include <Arduino.h>

const int networkLedPin = 2;

const int lcdBrightnessPin = 15;

const int pressureSensorPin = 34;
const int currentTransformerSensorPin = 35;

const int lowLedPin = 12;
const int highLedPin = 13;

const int highFloatSwitch = 23;
const int lowFloatSwitch = 18;

const int alarmBuzzerPin = 25;

const String mqttHost = "10.0.0.4";


const unsigned long lcdDelayTime = 10000; // lcd: show ssid and ip for 10 seconds; after 10 seconds show low/high alarm states and water level
const unsigned long lcdBacklightOffTime = 600000;  // turn off lcd backlight after 10 minutes
