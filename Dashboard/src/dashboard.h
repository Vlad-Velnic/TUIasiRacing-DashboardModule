#pragma once

// Libraries included

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <SPI.h>

// Numeric constants

#define NUM_LEDS 8   // Number of LEDs in the RPM strip
#define MAX_RPM 8000 // Max engine RPM
#define MIN_RPM 4000 // Min engine RPM

// Pins

#define RPM_PIN 5      // Data pin to LED strip for RPM
#define SD_CS_PIN 15   // Chip select pin for SD writer
#define SD_SCK_PIN 18  // SCK pin for SD writer
#define SD_MISO_PIN 19 // MISO pin for SD writer
#define SD_MOSI_PIN 23 // MOSI pin for SD writer

// Global variables and objects

extern RTC_DS1307 rtc;
extern DateTime timestamp;
extern Adafruit_NeoPixel trmetru;
extern int currentRPM;
extern String filename;
extern File logFile;

// Methods

void printTime();
void printDateAndTime();
void showRPM(int currentRPM);
String pad(int number);