#include "dashboard.h"

RTC_DS1307 rtc;
DateTime timestamp(1999, 1, 1, 0, 0, 0); // initialized with an absurd value
Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;
String filename = "/dump";
File logFile;

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
    //logFile.close();
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
  
  logFile.println("end");
  logFile.flush();
}
