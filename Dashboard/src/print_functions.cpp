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

void updateDisplay()
{
    display.clearDisplay();

    // GEAR - stânga sus
    for (int dx = 0; dx <= 1; dx++) {
        for (int dy = 0; dy <= 1; dy++) {
          display.setCursor(0 + dx, 0 + dy);
          display.print(currentGear);
        }
    }

    // Line 1 - Last Lap Time (dreapta sus)
    display.setTextSize(1);
    int textX = SCREEN_WIDTH - (6 * 1 * 10); // adjust width for ~10 chars
    display.setCursor(textX, 0);
    display.print(lastLapTime);

    // Line 2 - TEMP (dreapta mijloc)
    display.setTextSize(1); // mai mare
    display.setCursor(textX, 15);
    display.print(String(currentTemp, 1) + char(176)); // ex: 85.5

    // Line 3 - Battery Voltage (dreapta jos)
    display.setTextSize(1);
    display.setCursor(textX, 40);
    display.print(currentBatteryVoltage);
}