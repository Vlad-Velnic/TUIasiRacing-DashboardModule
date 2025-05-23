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

  display.setCursor(display.getCursorX() - 5, 0);
  display.print(':');
  display.setCursor(display.getCursorX() - 3, 0);
  if (seconds < 10)
    display.print('0');
  display.print(seconds);

  display.setCursor(display.getCursorX() - 3, 0);
  display.print(':');
  display.setCursor(display.getCursorX() - 3, 0);
  display.print(millisecs);

  // Engine Temp and Battery Voltage
  display.setTextSize(1);
  display.setCursor(textX + 5, 55);
  display.print(currentTemp);
  display.drawCircle(display.getCursorX() + 2, display.getCursorY() - 3, 2, SSD1306_WHITE);

  display.setCursor(textX + 26, 55);
  display.print(currentBatteryVoltage, 1);
  display.print('V');

  display.display();
}

// Fixed updateDisplay2 function
void updateDisplay2()
{
  display.clearDisplay();

  // GEAR - large, left side
  display.setTextSize(6);  // Reduced from 9 to fit better
  display.setCursor(5, 10); // Adjusted position
  display.print(currentGear);

  // Time display - right side
  display.setTextSize(1);
  
  unsigned int minutes = (ms / 60000) % 60;
  unsigned int seconds = (ms / 1000) % 60;
  unsigned int millisecs = (ms / 100) % 10; // Changed to show tenths

  // Position time display on right side
  display.setCursor(80, 5);
  if (minutes < 10) display.print('0');
  display.print(minutes);
  display.print(':');
  if (seconds < 10) display.print('0');
  display.print(seconds);
  display.print('.');
  display.print(millisecs);

  // Temperature - bottom right
  display.setCursor(80, 25);
  display.print(currentTemp, 0); // No decimal places
  display.print((char)247); // Degree symbol
  display.print('C');

  // Battery voltage - bottom right
  display.setCursor(80, 40);
  display.print(currentBatteryVoltage, 1);
  display.print('V');

  display.display();
}

// Alternative cleaner version
void updateDisplayClean()
{
  display.clearDisplay();
  
  // Set white text
  display.setTextColor(SSD1306_WHITE);

  // Large gear number on left
  display.setTextSize(8);
  display.setCursor(0, 5);
  display.print(currentGear);

  // Right side time
  display.setTextSize(2);
  
  // Time at top right
  unsigned long totalMs = ms;
  unsigned int mins = (totalMs / 60000) % 60;
  unsigned int secs = (totalMs / 1000) % 60;
  unsigned int tenths = (totalMs / 100) % 10;
  
  display.setCursor(50, 8);
  display.printf("%01d:%02d:%d", mins, secs, tenths);

  // Right side temp and voltage
  display.setTextSize(1);

  // Temperature
  display.setCursor(100, 42);
  display.printf("%.0f%cC", currentTemp, 247);
  
  // Battery voltage
  display.setCursor(95, 57);
  display.printf("%.1fV", currentBatteryVoltage);

  display.display();
}

void displaySal() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Salut!");
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
  Serial.print('\n');
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
}