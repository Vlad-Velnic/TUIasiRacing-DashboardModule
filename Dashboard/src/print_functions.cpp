#include "dashboard.h"

void printTime()
{
    // Displays "hour:minute:second" on serial

    Serial.print(timestamp.hour());
    Serial.print(':');
    Serial.print(timestamp.minute());
    Serial.print(':');
    Serial.print(timestamp.second());
}

void printDateAndTime()
{
    // Displays "day/month/year - hour:minute:second" on serial

    Serial.print(timestamp.day());
    Serial.print('/');
    Serial.print(timestamp.month());
    Serial.print('/');
    Serial.print(timestamp.year());
    Serial.print(" - ");

    printTime();
    // Serial.print(timestamp.hour());
    // Serial.print(':');
    // Serial.print(timestamp.minute());
    // Serial.print(':');
    // Serial.print(timestamp.second());
}