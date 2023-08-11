#include <Arduino.h>

const int networkLedPin = 2;

const int lcdBrightnessPin = 15;

const int trigPin = 17; // trigger pin for JSN-SR04T
const int echoPin = 16; // echo pin for JSN-SR04T

const int lowLedPin = 12;
const int highLedPin = 13;

const int highFloatSwitch = 23;
const int lowFloatSwitch = 5;  // TODO: change this pin to 18;  5 outputs PWM signal at boot, strapping pin

const int alarmBuzzerPin = 25;

const String mqttHost = "10.0.0.4";


const unsigned long lcdDelayTime = 10000; // lcd: show ssid and ip for 10 seconds; after 10 seconds show low/high alarm states and water level
const unsigned long lcdBacklightOffTime = 600000;  // turn off lcd backlight after 10 minutes
