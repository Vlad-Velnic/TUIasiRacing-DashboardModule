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

// final values taken from maps
#define LEFT_GATE_LAT 46.525579
#define LEFT_GATE_LON 26.942636
#define RIGHT_GATE_LAT 46.525648
#define RIGHT_GATE_LON 26.942909

// Pins

#define RPM_PIN GPIO_NUM_15 // Data pin to LED strip for RPM

#define SD_CS_PIN GPIO_NUM_5   // Chip select pin for SD writer
#define SD_SCK_PIN GPIO_NUM_18  // SCK pin for SD writer
#define SD_MISO_PIN GPIO_NUM_19 // MISO pin for SD writer
#define SD_MOSI_PIN GPIO_NUM_23 // MOSI pin for SD writer

#define TX_GPIO_NUM GPIO_NUM_33 // TX for CAN
#define RX_GPIO_NUM GPIO_NUM_32 // RX for CAN

// Epntru display sunt urmatorii pini:
// CS 15 la fel ca in KiCAD
// DC 13 din KiCAD este MOSI
// RES 14 din KiCAD este SCLK
// SDA -
// SCLK -
#define OLED_CS GPIO_NUM_15   // Chip Select
#define OLED_DC GPIO_NUM_16  // Data/Command
#define OLED_RST GPIO_NUM_14 // Reset

#define RTC_SDA GPIO_NUM_21 // Default I2C
#define RTC_SCL GPIO_NUM_22 // Default I2C

// Structs and Classes

struct GPSPoint
{
    double lat;
    double lon;
    double timestamp; // in unix time + milis offset
    double speed; // in km/h 
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
extern float currentTemp;
extern double lastLapTime;
extern float currentBatteryVoltage;
extern GPSPoint gateL;
extern GPSPoint gateR;
extern GPSPoint prevLocation;
extern GPSPoint currLocation;
extern double prevTime;
extern double currTime;
extern uint32_t rtcBase;
extern unsigned long millisBase;
extern double timestamp;


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