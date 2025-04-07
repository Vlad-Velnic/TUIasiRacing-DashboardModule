#include "dashboard.h"

RTC_DS1307 rtc;
DateTime timestamp(1999, 1, 1, 0, 0, 0); // initialized with an absurd value
Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;
String filename = "/dump";
File logFile;
u_int16_t status;

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

  // SETUP DONE
  Serial.print("/nInitialization done at: ");
  printTime();
  Serial.println("/n********LOOP Starts********/n/n");
}

void loop()
{
  timestamp = rtc.now();

  // Log on SD
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(1000)) == ESP_OK)
  {
    // Log basic CAN info
    logFile.print("Time: ");
    logFile.print(pad(timestamp.hour()));
    logFile.print(":");
    logFile.print(pad(timestamp.minute()));
    logFile.print(":");
    logFile.print(pad(timestamp.second()));
    logFile.print(" | ID: 0x");
    logFile.print(rx_msg.identifier, HEX);
    logFile.print(" | DLC: ");
    logFile.print(rx_msg.data_length_code);
    logFile.print(" | Data: ");

    // Log each byte
    for (int i = 0; i < rx_msg.data_length_code; ++i)
    {
      if (rx_msg.data[i] < 16)
        logFile.print("0");
      logFile.print(rx_msg.data[i], HEX);
      logFile.print(" ");
    }
    logFile.println();
    logFile.flush(); // Ensure data is written
  }
}
