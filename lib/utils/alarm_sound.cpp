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
    tone(alarmBuzzerPin, 440, 500);
    delay(500);
    tone(alarmBuzzerPin, 440, 500);
    delay(500);
    tone(alarmBuzzerPin, 440, 500);
}
