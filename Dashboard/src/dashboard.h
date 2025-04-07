#pragma once

//Libraries included
#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

//Global variables
RTC_DS1307 rtc;
DateTime timestamp;

//Methods
void display_time();