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

Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;

String filename = "/dump";
File logFile;
char logLine[128] = "DEFAULT MESSAGE";

u_int16_t status;
WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);

short int currentGear = 0;
float currentTemp = 0;
float currentBatteryVoltage = 0;

GPSPoint gateL = {LEFT_GATE_LAT, LEFT_GATE_LON, 0};
GPSPoint gateR = {RIGHT_GATE_LAT, RIGHT_GATE_LON, 0};
GPSPoint prevLocation;
GPSPoint currLocation;

void setup()
{
  Serial.begin(115200);
  RTC_Wire.begin(RTC_SDA, RTC_SCL);
  ArduinoOTA.begin(); // Over the air updates

  // Initialize RTC
  while (!rtc.begin(&RTC_Wire))
  {
    Serial.println("RTC initialization failed");
    sleep(10);
  }
  rtcBase = rtc.now().unixtime();
  millisBase = millis();

  // Initialize TRMETRU
  trmetru.begin();
  trmetru.show();

  // Initialize SD writer
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  while (!SD.begin(SD_CS_PIN))
  {
    Serial.println("SD card initialization failed");
    sleep(10);
  }

  // Initialize CAN
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
  while (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println("Oled display initialization failed");
    sleep(10);
  }

  // Initialize WIFI and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Create logfile
  dateAndTime = rtc.now();
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

  // SETUP DONE
  Serial.print("/nInitialization done at: ");
  printTime();
  Serial.println("/n******** Loop Starts ********/n/n");
}

void loop()
{
  timestamp = rtc.now().unixtime() + ((millis() - millisBase) % 1000) / 1000.0;

  // Log on SD and send on MQTT
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(1000)) == ESP_OK)
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
      currLocation.timestamp = rtc.now().unixtime() + ((millis() - millisBase) % 1000) / 1000.0;
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

    // Read current Batery Voltage
    if (rx_msg.identifier == VOLT_CAN_ID)
    {
      currentBatteryVoltage = ((rx_msg.data[2] << 8) | rx_msg.data[4]) / 10.0;
    }

    // Display update
    updateDisplay();

    // Write on SD
    if (logFile)
    {
      logFile.println(logLine);
      // logFile.flush(); // only for instant write on the SD
    }

    // Publish on MQTT
    if (client.connected())
    {
      client.publish("canbus/log", logLine);
    }
  }
  ArduinoOTA.handle();
}
