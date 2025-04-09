#pragma once

// Libraries included

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <SPI.h>
#include "driver/can.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>

// Constants

#define NUM_LEDS 8 // Number of LEDs in the RPM strip

#define MAX_RPM 8000 // Max engine RPM
#define MIN_RPM 4000 // Min engine RPM

#define ssid "Numele_Retelei_Tale"
#define password "Parola_WiFi"
#define mqtt_server "192.168.1.100"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define RPM_CAN_ID 0x05F0
#define GEAR_CAN_ID 0x0115
#define TEMP_CAN_ID 0x05F2
#define VOLT_CAN_ID 0x05F3
#define GPS_CAN_ID 0x0116

#define LEFT_GATE_LAT 47.123213
#define LEFT_GATE_LON 27.123123
#define RIGHT_GATE_LAT 47.234232
#define RIGHT_GATE_LON 27.231231

// Pins

#define RPM_PIN 5 // Data pin to LED strip for RPM

#define SD_CS_PIN 15   // Chip select pin for SD writer
#define SD_SCK_PIN 18  // SCK pin for SD writer
#define SD_MISO_PIN 19 // MISO pin for SD writer
#define SD_MOSI_PIN 23 // MOSI pin for SD writer

#define TX_GPIO_NUM GPIO_NUM_14 // TX for CAN
#define RX_GPIO_NUM GPIO_NUM_27 // RX for CAN

#define OLED_CS 5   // Chip Select
#define OLED_DC 16  // Data/Command
#define OLED_RST 17 // Reset

// Structs and Classes

struct GPSPoint
{
    double lat;
    double lon;
    uint32_t timestamp; // in unix time
    double speed;
};

// Global variables and objects

extern RTC_DS1307 rtc;
extern DateTime dateAndTime;
extern Adafruit_NeoPixel trmetru;
extern int currentRPM;
extern String filename;
extern File logFile;
extern u_int16_t status;
extern WiFiClient espClient;
extern PubSubClient client;
extern char logLine[128];
extern Adafruit_SSD1306 display;
extern short int currentGear;
extern int currentTemp;
extern uint32_t lastLapTime;
extern float currentBatteryVoltage;
extern GPSPoint gateL;
extern GPSPoint gateR;
extern GPSPoint prevLocation;
extern GPSPoint currLocation;
extern uint32_t prevTime;
extern uint32_t currTime;
extern uint32_t rtcBase;
extern unsigned long millisBase;
extern unsigned long timestamp;


// Methods

void printTime();
void printDateAndTime();
void showRPM(int currentRPM);
String pad(int number);
void setup_wifi();
void updateDisplay();
bool checkGateCrossing(GPSPoint prev, GPSPoint curr, double &newLapTime);
int orientation(GPSPoint p, GPSPoint q, GPSPoint r);
bool doIntersect(GPSPoint p1, GPSPoint q1);
bool onSegment(GPSPoint p, GPSPoint q, GPSPoint r);
bool getIntersectionTime(GPSPoint prev, GPSPoint curr);

// CAN config
static const can_general_config_t g_config = {.mode = TWAI_MODE_NO_ACK, .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM, .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED, .tx_queue_len = 1, .rx_queue_len = 5, .alerts_enabled = TWAI_ALERT_ALL, .clkout_divider = 0, .intr_flags = ESP_INTR_FLAG_LEVEL1};
static const can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();