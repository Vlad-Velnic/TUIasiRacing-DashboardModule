#include "dashboard.h"

// Initializations
RTC_DS1307 rtc;
DateTime timestamp(1999, 1, 1, 0, 0, 0); // initialized with an absurd value
Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;
String filename = "/dump";
File logFile;
u_int16_t status;
WiFiClient espClient;
PubSubClient client(espClient);
char logLine[128] = "DEFAULT MESSAGE";

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  // Initialize RTC
  while (!rtc.begin())
  {
    Serial.println("Nu s-a putut initializa RTC-ul!");
    sleep(10);
  }
  timestamp = rtc.now();

  // Initialize TRMETRU
  trmetru.begin();
  trmetru.show();

  // Initialize SD writer
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  while (!SD.begin(SD_CS_PIN))
  {
    Serial.println("SD card initialization failed!");
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

  // Create logfile
  filename = "/log_" +
             String(timestamp.year()) + "-" +
             pad(timestamp.month()) + "-" +
             pad(timestamp.day()) + "_" +
             pad(timestamp.hour()) + "-" +
             pad(timestamp.minute()) + "-" +
             pad(timestamp.second()) + ".txt";

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

  // Initialize WIFI and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // SETUP DONE
  Serial.print("/nInitialization done at: ");
  printTime();
  Serial.println("/n******** Loop Starts ********/n/n");
}

void loop()
{
  timestamp = rtc.now();

  // Log on SD and send on MQTT
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(1000)) == ESP_OK)
  {
    // Build the "logline"
    int offset = snprintf(logLine, sizeof(logLine), "%lu,%lX,", timestamp.unixtime(), rx_msg.identifier);
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
}
