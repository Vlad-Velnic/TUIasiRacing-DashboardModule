#include "dashboard.h"


Dashboard::Dashboard() : rtcWire(1),
                        rpmLeds(Config::NUM_LEDS, Config::RPM_PIN, NEO_GRB + NEO_KHZ800),
                        spiOled(HSPI),
                        spiSDCard(VSPI),
                        display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &spiOled, Config::OLED_DC, -1, Config::OLED_CS),
                        mqttClient(wifiClient)
{
   // Initialize GPS gate points (unchanged)
   gateL = GPSPoint(Config::LEFT_GATE_LAT, Config::LEFT_GATE_LON, 0.0, 0.0);
   gateR = GPSPoint(Config::RIGHT_GATE_LAT, Config::RIGHT_GATE_LON, 0.0, 0.0);


   // Pre-compute gate vector for lap timing (unchanged)
   gateVectorX = gateR.lon - gateL.lon;
   gateVectorY = gateR.lat - gateL.lat;
}


bool Dashboard::initialize()
{
   // This MUST run before any debug prints
   Serial.begin(115200); 
   if (Config::DEBUG_SERIAL) Serial.println("Initializing dashboard...");

   // Initialize hardware components first
   // These are critical and should run to completion.
   rpmLeds.begin();
   rpmLeds.show(); // Turn off LEDs

   if (!initializeRTC()) {
       // Non-fatal, but time will be wrong
   }

   if (!initializeSD())
   {
       if (Config::DEBUG_SERIAL) Serial.println("WARNING: SD card initialization failed");
       // Non-fatal, but logging won't work
   }

   if (!initializeDisplay())
   {
       if (Config::DEBUG_SERIAL) Serial.println("ERROR: Display initialization failed");
       return false; // Fatal
   }

   if (!initializeCAN())
   {
       if (Config::DEBUG_SERIAL) Serial.println("ERROR: CAN initialization failed");
       return false; // Fatal
   }

   // Start non-blocking network initialization
   startWiFi(); 

   // Setup MQTT (server only, doesn't connect yet)
   mqttClient.setServer(Config::MQTT_SERVER, 1883);

   // Setup OTA
   ArduinoOTA.begin();

   // List SD files for debugging
   listSDFiles();

   if (Config::DEBUG_SERIAL) Serial.println("Initialization complete. Main loop starting.");
   return true;
}

// NEW: Handles all non-blocking network logic
void Dashboard::updateNetwork() {
   unsigned long currentMillis = millis();

   // 1. Check WiFi Connection Status
   if (WiFi.status() != WL_CONNECTED) {
       // Not connected, check if it's time to retry
       if (currentMillis - lastWifiAttempt > Config::WIFI_RETRY_INTERVAL) {
           if (Config::DEBUG_SERIAL) Serial.println("Retrying WiFi connection...");
           WiFi.begin(); // Re-issue begin
           lastWifiAttempt = currentMillis;
       }
   } else {
       // WiFi is connected!
       // 2. Check NTP Sync Status (only run once)
       if (!timeIsSynced) {
           if (syncNTP()) {
               // NTP Success!
               timeIsSynced = true;
           } else {
               // NTP failed. Check for timeout.
               if (currentMillis - ntpAttemptStart > Config::NTP_SYNC_TIMEOUT) {
                   if (Config::DEBUG_SERIAL) Serial.println("NTP sync timed out. Using RTC time.");
                   timeIsSynced = true; // Give up on NTP and set flag to true
               }
           }
       }
   }

   // 3. Create Log File (only run once)
   // This will run as soon as NTP is synced OR NTP times out.
   if (timeIsSynced && !logFileCreated) {
       createLogFile();
   }

   // 4. Handle MQTT Client
   // Needs to be called on every loop for background tasks
   if (WiFi.status() == WL_CONNECTED) {
        mqttClient.loop();
   }
}


void Dashboard::update()
{
   unsigned long currentMillis = millis();

   // Handle non-blocking network connections (WiFi, NTP, MQTT)
   updateNetwork();

   // Process CAN messages (highest priority)
   processCANMessages();


   // Update display if needed
   if (displayNeedsUpdate || (currentMillis - lastDisplayUpdate >= Config::DISPLAY_UPDATE_INTERVAL))
   {
       lastDisplayUpdate = currentMillis;
       updateDisplayClean();
       displayNeedsUpdate = false;
   }


   // Publish MQTT data
   if (currentMillis - lastMQTTPublish >= Config::MQTT_PUBLISH_INTERVAL)
   {
       lastMQTTPublish = currentMillis;
       publishMQTT();
   }


   // Flush SD buffer
   if (currentMillis - lastSDFlush >= Config::SD_FLUSH_INTERVAL)
   {
       lastSDFlush = currentMillis;
       flushSDBuffer();
   }


   // Increase dateTime (only if time is synced)
   if (timeIsSynced && (currentMillis - lastSecond >= Config::SECOND))
   {
       lastSecond = currentMillis;
       dateTime++;
   }


   // Handle OTA updates
   ArduinoOTA.handle();
}


// UPDATED: More efficient queue-draining method
void Dashboard::processCANMessages()
{
   twai_message_t msg;

   // Process ALL available messages in the queue right now
   while (twai_receive(&msg, 0) == ESP_OK) { // 0 timeout = non-blocking
       if (Config::DEBUG_SERIAL && Config::DEBUG_CAN)
       {
           Serial.printf("Can message recived with ID: 0x%X, Data: ", msg.identifier);
           for (int i = 0; i < 8; i++)
           {
               Serial.printf("%02X ", msg.data[i]);
           }
           Serial.println();
       }
       handleCANMessage(msg);
   }
   // If no messages, this while loop is skipped instantly.
}


void Dashboard::handleCANMessage(const twai_message_t &msg)
{
   // Format log line for SD and MQTT
   formatLogLine(logLine, Config::LOG_LINE_SIZE, msg.identifier, msg.data, msg.data_length_code);


   // Log to SD (only if file is open)
   if (logFile) {
       logToSD(logLine);
   }

   // Process message based on ID
   switch (msg.identifier)
   {
   case Config::RPM_CAN_ID:
       processRPMMessage(msg);
       break;
   case Config::GEAR_CAN_ID:
       processGearMessage(msg);
       break;
   case Config::GPS_CAN_ID:
       processGPSMessage(msg);
       break;
   case Config::TEMP_CAN_ID:
       processTempMessage(msg);
       break;
   case Config::VOLT_CAN_ID:
       processVoltMessage(msg);
       break;
   }
}


void Dashboard::processRPMMessage(const twai_message_t &msg)
{
   int newRPM = (msg.data[6] << 8) | msg.data[7];
   if (newRPM != currentRPM)
   {
       currentRPM = newRPM;
       displayNeedsUpdate = true;


       if (currentRPM > Config::MIN_RPM)
       {
           showRPM(currentRPM);
       } else {
           showRPM(0); // Turn off LEDs if below min
       }
   }
}


void Dashboard::processGearMessage(const twai_message_t &msg)
{
   short int newGear = msg.data[4];
   if (newGear != currentGear)
   {
       currentGear = newGear;
       displayNeedsUpdate = true;
   }
}


void Dashboard::processGPSMessage(const twai_message_t &msg)
{
   // Store previous location
   prevLocation = currLocation;

   // Update current location
   currLocation.timestamp = dateTime + (millis() % 1000) / 1000.0;
   // Use new constants
   currLocation.lat = Config::GPS_BASE_LAT + (msg.data[2] << 16 | msg.data[1] << 8 | msg.data[0]) / Config::GPS_SCALE_FACTOR;
   currLocation.lon = Config::GPS_BASE_LON + (msg.data[5] << 16 | msg.data[4] << 8 | msg.data[3]) / Config::GPS_SCALE_FACTOR;
   currLocation.speed = msg.data[6];

   // Check if we've moved enough to check for gate crossing
   if (TinyGPSPlus::distanceBetween(prevLocation.lat, prevLocation.lon, currLocation.lat, currLocation.lon) > 1.5)
   {
       if (getIntersectionTime(prevLocation, currLocation))
       {
           // Update lap time
           lastLapTime = currTime - prevTime;
           prevTime = currTime;
           displayNeedsUpdate = true;
       }
   }
}


void Dashboard::processTempMessage(const twai_message_t &msg)
{
   // Use new constant
   float newTemp = ((((msg.data[6] << 8) | msg.data[7]) - 32) / 1.8) / Config::TEMP_SCALE_FACTOR;
   if (abs(newTemp - currentTemp) > 0.5)
   {
       currentTemp = newTemp;
       displayNeedsUpdate = true;
   }
}


void Dashboard::processVoltMessage(const twai_message_t &msg)
{
   // Use new constant
   float newVoltage = ((msg.data[2] << 8) | msg.data[4]) / Config::VOLT_SCALE_FACTOR;
   if (abs(newVoltage - currentBatteryVoltage) > 0.1)
   {
       currentBatteryVoltage = newVoltage;
       displayNeedsUpdate = true;
   }
}


void Dashboard::updateDisplayClean()
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
   unsigned long totalMs = lastLapTime * 1000; // Assuming lastLapTime is in seconds
   unsigned int mins = (totalMs / 60000) % 60;
   unsigned int secs = (totalMs / 1000) % 60;
   unsigned int tenths = (totalMs / 100) % 10;


   display.setCursor(50, 8);
   display.printf("%01d:%02d:%d", mins, secs, tenths);


   // Right side temp and voltage
   display.setTextSize(1);


   // Temperature
   display.setCursor(100, 42);
   display.printf("%.0f%cC", currentTemp, 247); // 247 is degree symbol


   // Battery voltage
   display.setCursor(95, 57);
   display.printf("%.1fV", currentBatteryVoltage);


   display.display();
}


void Dashboard::showRPM(int rpm)
{
   // Map RPM to LED count
   int ledCount = map(rpm, Config::MIN_RPM, Config::MAX_RPM, 0, Config::NUM_LEDS);
   ledCount = constrain(ledCount, 0, Config::NUM_LEDS);


   // Set colors based on RPM range
   for (int i = 0; i < Config::NUM_LEDS; i++)
   {
       if (i < ledCount)
       {
           // Progressive color: green -> yellow -> red
           if (i < Config::NUM_LEDS / 3)
           {
               rpmLeds.setPixelColor(i, rpmLeds.Color(0, 255, 0)); // Green
           }
           else if (i < 2 * Config::NUM_LEDS / 3)
           {
               rpmLeds.setPixelColor(i, rpmLeds.Color(255, 255, 0)); // Yellow
           }
           else
           {
               rpmLeds.setPixelColor(i, rpmLeds.Color(255, 0, 0)); // Red
           }
       }
       else
       {
           rpmLeds.setPixelColor(i, rpmLeds.Color(0, 0, 0)); // Off
       }
   }


   rpmLeds.show();
}


void Dashboard::logToSD(const char *message)
{
   size_t msgLen = strlen(message);


   // If buffer would overflow, flush it first
   if (sdBufferPos + msgLen + 2 > Config::SD_BUFFER_SIZE)
   {
       flushSDBuffer();
   }


   // Add message to buffer
   strcpy(sdBuffer + sdBufferPos, message);
   sdBufferPos += msgLen;
   sdBuffer[sdBufferPos++] = '\n';
   sdBuffer[sdBufferPos] = '\0';
}


void Dashboard::flushSDBuffer()
{
   if (sdBufferPos > 0 && logFile)
   {
       logFile.print(sdBuffer);
       logFile.flush();
       sdBufferPos = 0;
   }
}


// UPDATED: Non-blocking reconnect logic
void Dashboard::publishMQTT()
{
   unsigned long currentMillis = millis();
   
   if (!mqttClient.connected())
   {
       // Only try to reconnect if WiFi is on and timer has elapsed
       if (WiFi.status() == WL_CONNECTED && (currentMillis - lastMqttAttempt > Config::MQTT_RETRY_INTERVAL))
       {
           if (Config::DEBUG_SERIAL) Serial.println("Attempting MQTT connection...");
           lastMqttAttempt = currentMillis;
           // This connect call IS still blocking, but now it only runs every 5s
           if (mqttClient.connect("tuiasi-dashboard")) {
               if (Config::DEBUG_SERIAL) Serial.println("MQTT connected");
           } else {
               if (Config::DEBUG_SERIAL) Serial.print("MQTT failed, rc=");
               if (Config::DEBUG_SERIAL) Serial.println(mqttClient.state());
           }
       }
   }
   else
   {
       // If connected, publish
       mqttClient.publish("canbus/log", logLine);
   }
}


void Dashboard::formatLogLine(char *buffer, size_t size, uint32_t id, const uint8_t *data, uint8_t length)
{
   // Format: timestamp,id,data
   int offset = snprintf(buffer, size, "%.3f,%lX,",
                         dateTime + (millis() % 1000) / 1000.0, id);


   // Add data bytes
   for (int i = 0; i < length && offset < size - 3; i++)
   {
       offset += snprintf(buffer + offset, size - offset, "%02X", data[i]);
   }
}


void Dashboard::listSDFiles()
{
   if (!SD.begin(Config::SD_CS_PIN)) {
       if (Config::DEBUG_SERIAL) Serial.println("SD Card not present for file listing.");
       return;
   }
   
   if (Config::DEBUG_SERIAL) Serial.println("Files on SD card:");
   File root = SD.open("/");
   if(!root){
       if (Config::DEBUG_SERIAL) Serial.println("Failed to open root directory");
       return;
   }

   File file = root.openNextFile();
   while (file)
   {
       if (!file.isDirectory())
       {
           if (Config::DEBUG_SERIAL) Serial.print("  ");
           if (Config::DEBUG_SERIAL) Serial.print(file.name());
           if (Config::DEBUG_SERIAL) Serial.print("  (");
           if (Config::DEBUG_SERIAL) Serial.print(file.size());
           if (Config::DEBUG_SERIAL) Serial.println(" bytes)");
       }
       file = root.openNextFile();
   }


   file.close();
   root.close();
}

// --- Lap Timing Geometry Functions (Unchanged) ---

bool Dashboard::getIntersectionTime(const GPSPoint &prev, const GPSPoint &curr)
{
   if (!doIntersect(prev, curr))
   {
       return false;
   }


   double x1 = prev.lon, y1 = prev.lat;
   double x2 = curr.lon, y2 = curr.lat;
   double x3 = gateL.lon, y3 = gateL.lat;
   double x4 = gateR.lon, y4 = gateR.lat;


   double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
   if (denom == 0)
   {
       return false;
   }


   double interLon = ((x1 * y2 - y1 * x2) * (x3 - x4) -
                      (x1 - x2) * (x3 * y4 - y3 * x4)) /
                     denom;
   double interLat = ((x1 * y2 - y1 * x2) * (y3 - y4) -
                      (y1 - y2) * (x3 * y4 - y3 * x4)) /
                     denom;


   double distToInter = hypot(interLon - prev.lon, interLat - prev.lat);
   double avgSpeed = (prev.speed + curr.speed) / 2.0;
   if (avgSpeed < 1)
   {
       avgSpeed = 1; // avoid division by 0
   }


   double ratio = distToInter / (avgSpeed / 3.6); // km/h → m/s
   currTime = prev.timestamp + ratio * (curr.timestamp - prev.timestamp);


   return true;
}


bool Dashboard::doIntersect(const GPSPoint &p1, const GPSPoint &q1)
{
   int o1 = orientation(p1, q1, gateL);
   int o2 = orientation(p1, q1, gateR);
   int o3 = orientation(gateL, gateR, p1);
   int o4 = orientation(gateL, gateR, q1);


   if (o1 != o2 && o3 != o4)
   {
       return true;
   }


   if (o1 == 0 && onSegment(p1, gateL, q1))
       return true;
   if (o2 == 0 && onSegment(p1, gateR, q1))
       return true;
   if (o3 == 0 && onSegment(gateL, p1, gateR))
       return true;
   if (o4 == 0 && onSegment(gateL, q1, gateR))
       return true;


   return false;
}


int Dashboard::orientation(const GPSPoint &p, const GPSPoint &q, const GPSPoint &r)
{
   double val = (q.lon - p.lon) * (r.lat - q.lat) - (q.lat - p.lat) * (r.lon - q.lon);
   if (val == 0.0)
   {
       return 0; // colinear
   }
   return (val > 0.0) ? 1 : 2; // clock or counterclockwise
}


bool Dashboard::onSegment(const GPSPoint &p, const GPSPoint &q, const GPSPoint &r)
{
   return q.lon <= fmax(p.lon, r.lon) && q.lon >= fmin(p.lon, r.lon) &&
          q.lat <= fmax(p.lat, r.lat) && q.lat >= fmin(p.lat, r.lat);
}


// Global dashboard instance
Dashboard dashboard;