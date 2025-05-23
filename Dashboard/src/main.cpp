#include "dashboard.h"

// Initializations
RTC_DS1307 rtc;
TwoWire RTC_Wire(1);
DateTime dateAndTime = {1999, 0, 0, 0, 0, 0}; // initialized with an absurd value
double timestamp;
double prevTime;
double currTime;
uint32_t rtcBase;
unsigned long millisBase;
double lastLapTime = 0;
DateTime ntpTime;
long ms = 0;

Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;

String filename = "/dump";
File logFile;
char logLine[128] = "DEFAULT MESSAGE";

u_int16_t status;
WiFiClient espClient;
PubSubClient client(espClient);

SPIClass SPI_oled(HSPI);
SPIClass SPI_sdcard(VSPI);

Adafruit_SSD1306 display(128, 64, &SPI_oled, OLED_DC, -1, OLED_CS);

short int currentGear = 2;
float currentTemp = 80;
float currentBatteryVoltage = 14.2;

GPSPoint gateL = {LEFT_GATE_LAT, LEFT_GATE_LON, 0};
GPSPoint gateR = {RIGHT_GATE_LAT, RIGHT_GATE_LON, 0};
GPSPoint prevLocation;
GPSPoint currLocation;

void setup()
{
  Serial.begin(115200);

  // Initialize TRMETRU
  trmetru.begin();
  trmetru.show();

  //Initialize SD writer
  SPI_sdcard.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  while (!SD.begin(SD_CS_PIN))
  {
    Serial.println("SD card initialization failed");
    delay(10);
  }
  Serial.println("SD card initialization successful");

  //Initialize CAN
  status = can_driver_install(&g_config, &t_config, &f_config);
  if (status == ESP_OK)
  {
    Serial.println("Can driver installed");
  }
  else
  {
    Serial.println("Can driver installation failed with errors");
  }

  status = can_start();
  if (status == ESP_OK)
  {
    Serial.println("Can started");
  }
  else
  {
    Serial.println("Can starting procedure failed with errors");
  }

  // Iitialize DISPLAY
  SPI_oled.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
  while (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println("Oled display initialization failed");
    delay(10);
  }
  Serial.println("Display initialization successful");
  updateDisplay2();

  // Initialize WIFI and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  ArduinoOTA.begin(); // Over the air updates
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.println("Eroare la NTP");
    delay(100);
  }
  DateTime temp(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour + 1,
      timeinfo.tm_min,
      timeinfo.tm_sec);
  ntpTime = temp;


  // Create logfile
  dateAndTime = ntpTime;
  printDateAndTime();
  filename = "/log_" +
             String(dateAndTime.year()) + "-" +
             pad(dateAndTime.month()) + "-" +
             pad(dateAndTime.day()) + "_" +
             pad(dateAndTime.hour()) + "-" +
             pad(dateAndTime.minute()) + "-" +
             pad(dateAndTime.second()) + ".txt";

  logFile = SD.open(filename, FILE_WRITE);
  if (logFile)
  {
    Serial.print("Logging data to: ");
    Serial.println(filename);
    // logFile.close();
  }
  else
  {
    Serial.println("Error opening file.");
  }

  listSDFiles();    // List existing files
  //testSDWrite();    // Test write capability
  //listSDFiles();

  // SETUP DONE
  Serial.printf("\nInitialization done at: ");
  printTime();
  Serial.printf("\n******** Loop Starts ********\n\n");
}

void loop()
{
  timestamp = ntpTime.unixtime() + (millis() % 1000) / 1000.0;
  lastLapTime = timestamp;

  // Handle CAN messages
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) == ESP_OK)
  {
    // Build the "logline"
    int offset = snprintf(logLine, sizeof(logLine), "%lu,%lX,", timestamp, rx_msg.identifier);
    for (int i = 0; i < rx_msg.data_length_code; i++)
    {
      offset += snprintf(logLine + offset, sizeof(logLine) - offset, "%02X", rx_msg.data[i]);
    }

    // Show current RPM
    if (rx_msg.identifier == RPM_CAN_ID)
    {
      currentRPM = (rx_msg.data[6] << 8) | rx_msg.data[7];
      if (currentRPM > MIN_RPM)
      {
        showRPM(currentRPM);
      }
    }

    // Read current Gear
    if (rx_msg.identifier == GEAR_CAN_ID)
    {
      currentGear = rx_msg.data[6];
    }

    // Read GPS data
    if (rx_msg.identifier == GPS_CAN_ID)
    {
      currLocation.timestamp = ntpTime.unixtime() + (millis() % 1000) / 1000.0;
      currLocation.lat = 46.0 + (rx_msg.data[2] << 16 | rx_msg.data[1] << 8 | rx_msg.data[0]) / 1000000.0;
      currLocation.lon = 26.0 + (rx_msg.data[5] << 16 | rx_msg.data[4] << 8 | rx_msg.data[3]) / 1000000.0;
      currLocation.speed = rx_msg.data[6];

      if (TinyGPSPlus::distanceBetween(prevLocation.lat, prevLocation.lon, currLocation.lat, currLocation.lon) > 1.5)
      {
        if (getIntersectionTime(prevLocation, currLocation))
        {
          // currTime updated
          lastLapTime = currTime - prevTime;
          prevTime = currTime;
        }
        prevLocation = currLocation;
      }
    }

    // Read current Engine Temperature
    if (rx_msg.identifier == TEMP_CAN_ID)
    {
      currentTemp = ((((rx_msg.data[6] << 8) | rx_msg.data[7]) - 32) / 1.8) / 10.0; // converts to Celsius
    }

    // Read current Battery Voltage
    if (rx_msg.identifier == VOLT_CAN_ID)
    {
      currentBatteryVoltage = ((rx_msg.data[2] << 8) | rx_msg.data[4]) / 10.0;
    }

    // Write on SD
    if (logFile)
    {
      logFile.println(logLine);
    }

    // Publish on MQTT
    if (client.connected())
    {
      client.publish("canbus/log", logLine);
    }
  }

  // Update display ONLY ONCE per loop
  ms = millis(); // Update ms before display update
  updateDisplayClean(); // Use the cleaner version
  
  ArduinoOTA.handle();
  
  // Small delay to prevent overwhelming the display
  //delay(50);
}