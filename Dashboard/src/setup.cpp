#include "dashboard.h"


bool Dashboard::initializeSD() {
   spiSDCard.begin(Config::SD_SCK_PIN, Config::SD_MISO_PIN, Config::SD_MOSI_PIN, Config::SD_CS_PIN);
   if (!SD.begin(Config::SD_CS_PIN)) {
       Serial.println("SD card initialization failed");
       return false;
   }
   Serial.println("SD card initialization successful");
   return true;
}


bool Dashboard::initializeCAN() {
   esp_err_t status = can_driver_install(&g_config, &t_config, &f_config);
   if (status != ESP_OK) {
       Serial.println("CAN driver installation failed");
       return false;
   }
  
   status = can_start();
   if (status != ESP_OK) {
       Serial.println("CAN start failed");
       return false;
   }
  
   Serial.println("CAN initialized successfully");
   return true;
}


bool Dashboard::initializeDisplay() {
   spiOled.begin(Config::OLED_CLK, -1, Config::OLED_MOSI, Config::OLED_CS);
   if (!display.begin(SSD1306_SWITCHCAPVCC)) {
       Serial.println("Display initialization failed");
       return false;
   }
  
   display.clearDisplay();
   display.setTextColor(SSD1306_WHITE);
   display.display();
  
   Serial.println("Display initialized successfully");
   return true;
}


bool Dashboard::initializeWiFi() {
   WiFi.begin();
  
   int attempts = 0;
   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
       delay(500);
       Serial.print(".");
       attempts++;
   }
  
   if (WiFi.status() != WL_CONNECTED) {
       Serial.println("WiFi connection failed");
       return false;
   }
  
   Serial.println("WiFi connected");
   Serial.print("IP address: ");
   Serial.println(WiFi.localIP());
  
   // Setup MQTT
   mqttClient.setServer(Config::MQTT_SERVER, 1883);
  
   // Setup OTA
   ArduinoOTA.begin();
  
   return true;
}


bool Dashboard::initializeNTP() {
   configTime(Config::GMT_OFFSET_SEC, Config::DAYLIGHT_OFFSET_SEC, Config::NTP_SERVER);
  
  
   int attempts = 0;
   while (!getLocalTime(&timeinfo) && attempts < 10) {
       Serial.println("Failed to obtain time");
       delay(500);
       attempts++;
   }
  
   if (!getLocalTime(&timeinfo)) {
       Serial.println("NTP time sync failed");
       return false;
   }
  
   dateTime = DateTime(
       timeinfo.tm_year + 1900,
       timeinfo.tm_mon + 1,
       timeinfo.tm_mday,
       timeinfo.tm_hour + 1,
       timeinfo.tm_min,
       timeinfo.tm_sec
   ).unixtime();
  
   Serial.println("NTP time synchronized");
   return true;
}


void Dashboard::createLogFile() {
   // Format filename with date and time
   DateTime dateTimeSD= DateTime(
       timeinfo.tm_year + 1900,
       timeinfo.tm_mon + 1,
       timeinfo.tm_mday,
       timeinfo.tm_hour + 1,
       timeinfo.tm_min,
       timeinfo.tm_sec
   );
   snprintf(filename, Config::FILENAME_SIZE,
       "/log_%04d-%02d-%02d_%02d-%02d-%02d.txt",
       dateTimeSD.year(), dateTimeSD.month(), dateTimeSD.day(),
       dateTimeSD.hour(), dateTimeSD.minute(), dateTimeSD.second());
  
   logFile = SD.open(filename, FILE_WRITE);
   if (logFile) {
       Serial.print("Logging data to: ");
       Serial.println(filename);
      
       // Write header
       logFile.println("timestamp,id,data");
   } else {
       Serial.println("Error opening log file");
   }
}
