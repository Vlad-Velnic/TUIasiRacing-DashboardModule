#include "dashboard.h"

void updateDisplay()
{
    display.clearDisplay();

  // GEAR 
  display.setTextSize(9); 
  display.setCursor(9, 0);
  display.print(currentGear);

  // Lap Time
  display.setTextSize(2);
  int textX = SCREEN_WIDTH - (6 * 1 * 10);
  display.setCursor(textX, 0);

  unsigned int minutes = (int)lastLapTime / 60;
  unsigned int seconds = (int)lastLapTime % 60;
  unsigned int millisecs = (int)((lastLapTime - (int)lastLapTime) * 10);

  display.setCursor(textX, 0);
  display.print(minutes);

  display.setCursor(display.getCursorX()-5, 0);
  display.print(':');
  display.setCursor(display.getCursorX()-3, 0);
  if (seconds < 10)
    display.print('0');
  display.print(seconds);

  display.setCursor(display.getCursorX()-3, 0);
  display.print(':');
  display.setCursor(display.getCursorX()-3, 0);
  display.print(millisecs);

  // Engine Temp and Battery Voltage
  display.setTextSize(1);
  display.setCursor(textX+5, 55);
  display.print(currentTemp);
  display.drawCircle(display.getCursorX() + 2, display.getCursorY() - 3, 2, SSD1306_WHITE);

  display.setCursor(textX+26, 55);
  display.print(currentBatteryVoltage, 1);
  display.print('V');

  display.display();
}

void printTime()
{
    // Displays "hour:minute:second" on serial

    Serial.print(dateAndTime.hour());
    Serial.print(':');
    Serial.print(dateAndTime.minute());
    Serial.print(':');
    Serial.print(dateAndTime.second());
}

void printDateAndTime()
{
    // Displays "day/month/year - hour:minute:second" on serial

    Serial.print(dateAndTime.day());
    Serial.print('/');
    Serial.print(dateAndTime.month());
    Serial.print('/');
    Serial.print(dateAndTime.year());
    Serial.print(" - ");

    printTime();
    // Serial.print(timestamp.hour());
    // Serial.print(':');
    // Serial.print(timestamp.minute());
    // Serial.print(':');
    // Serial.print(timestamp.second());
}