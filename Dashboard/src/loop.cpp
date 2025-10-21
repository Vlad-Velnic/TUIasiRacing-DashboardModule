#include "dashboard.h"


Dashboard::Dashboard() : rtcWire(1),
                        rpmLeds(Config::NUM_LEDS, Config::RPM_PIN, NEO_GRB + NEO_KHZ800),
                        spiOled(HSPI),
                        spiSDCard(VSPI),
                        display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &spiOled, Config::OLED_DC, -1, Config::OLED_CS),
                        mqttClient(wifiClient)
{
   // Initialize GPS gate points
   gateL = GPSPoint(Config::LEFT_GATE_LAT, Config::LEFT_GATE_LON, 0.0, 0.0);
   gateR = GPSPoint(Config::RIGHT_GATE_LAT, Config::RIGHT_GATE_LON, 0.0, 0.0);


   // Pre-compute gate vector for lap timing
   gateVectorX = gateR.lon - gateL.lon;
   gateVectorY = gateR.lat - gateL.lat;
}


bool Dashboard::initialize()
{
   Serial.begin(115200);
   Serial.println("Initializing dashboard...");


   // Initialize components
   rpmLeds.begin();
   rpmLeds.show();


   if (!initializeSD())
   {
       Serial.println("WARNING: SD card initialization failed");
   }


   if (!initializeCAN())
   {
       Serial.println("ERROR: CAN initialization failed");
       return false;
   }


   if (!initializeDisplay())
   {
       Serial.println("ERROR: Display initialization failed");
       return false;
   }


   if (!initializeWiFi())
   {
       Serial.println("WARNING: WiFi initialization failed");
   }


   if (!initializeNTP())
   {
       Serial.println("WARNING: NTP initialization failed");
   }


   // Create log file
   createLogFile();


   // List SD files for debugging
   listSDFiles();


   Serial.println("Initialization complete");
   return true;
}


void Dashboard::update()
{
   unsigned long currentMillis = millis();


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


   // Increase dateTime
   if (currentMillis - lastSecond >= Config::SECOND)
   {
       lastSecond = currentMillis;
       dateTime++;
   }


   // Handle OTA updates
   ArduinoOTA.handle();
}


void Dashboard::processCANMessages()
{
   twai_message_t msg;


   // Process up to 10 messages per loop to prevent blocking
   for (int i = 0; i < 10; i++)
   {
       if (twai_receive(&msg, 0) == ESP_OK)
       {
           if (vb)
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
       else
       {
           // break; // No more messages
       }
   }
}


void Dashboard::handleCANMessage(const twai_message_t &msg)
{
   // Format log line for SD and MQTT
   formatLogLine(logLine, Config::LOG_LINE_SIZE, msg.identifier, msg.data, msg.data_length_code);


   // Log to SD


   logToSD(logLine);


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
   currLocation.lat = 46.0 + (msg.data[2] << 16 | msg.data[1] << 8 | msg.data[0]) / 10000.0;
   currLocation.lon = 26.0 + (msg.data[5] << 16 | msg.data[4] << 8 | msg.data[3]) / 10000.0;
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
   float newTemp = ((((msg.data[6] << 8) | msg.data[7]) - 32) / 1.8) / 10.0;
   if (abs(newTemp - currentTemp) > 0.5)
   {
       currentTemp = newTemp;
       displayNeedsUpdate = true;
   }
}


void Dashboard::processVoltMessage(const twai_message_t &msg)
{
   float newVoltage = ((msg.data[2] << 8) | msg.data[4]) / 10.0;
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
   unsigned long totalMs = lastLapTime;
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


void Dashboard::publishMQTT()
{
   if (mqttClient.connected())
   {
       mqttClient.publish("canbus/log", logLine);
   }
   else
   {
       // Try to reconnect
       Serial.println("MQTT fail");
       if (WiFi.status() == WL_CONNECTED)
       {
           mqttClient.connect("");
       }
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
   Serial.println("Files on SD card:");
   File root = SD.open("/");
   File file = root.openNextFile();


   while (file)
   {
       if (!file.isDirectory())
       {
           Serial.print("  ");
           Serial.print(file.name());
           Serial.print("  (");
           Serial.print(file.size());
           Serial.println(" bytes)");
       }
       file = root.openNextFile();
   }


   file.close();
   root.close();
}


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
