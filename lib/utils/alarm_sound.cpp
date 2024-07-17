#include "../constants.h"


void startupSound()
{
    tone(alarmBuzzerPin, 440, 100);
    delay(200);
    tone(alarmBuzzerPin, 880, 100);
    delay(200);
    tone(alarmBuzzerPin, 1760, 100);
    delay(200);
    tone(alarmBuzzerPin, 660, 100);
}


void alarmSound()
{

    for (int hz = 440; hz < 880; hz++)
    {
        tone(alarmBuzzerPin, hz, 50);
        delay(5);
    }

    delay(1000);
    tone(alarmBuzzerPin, 100, 500);
    delay(1000);
    tone(alarmBuzzerPin, 100, 500);
    delay(1000);
    tone(alarmBuzzerPin, 100, 500);
}
