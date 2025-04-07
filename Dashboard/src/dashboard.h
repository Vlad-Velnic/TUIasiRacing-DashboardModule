#pragma once

// Libraries included

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>



// Numeric constants

#define NUM_LEDS    8        // Number of LEDs in the RPM strip
#define MAX_RPM     8000     // Max engine RPM
#define MIN_RPM     4000     // Min engine RPM



// Pins

#define RPM_PIN     5        // Data pin to LED strip for RPM



// Global variables and objects

extern RTC_DS1307 rtc;
extern DateTime timestamp; //initializat cu o valoare absurda
extern Adafruit_NeoPixel trmetru;
extern int currentRPM;



// Methods

void printTime();
void printDateAndTime();
void showRPM(int currentRPM);