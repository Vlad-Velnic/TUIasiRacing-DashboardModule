#include "dashboard.h"

RTC_DS1307 rtc;
DateTime timestamp(1999, 1, 1, 0, 0, 0); // initializat cu o valoare absurda
Adafruit_NeoPixel trmetru(NUM_LEDS, RPM_PIN, NEO_GRB + NEO_KHZ800);
int currentRPM = 0;


void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initializare RTC
  while (!rtc.begin()) {
    Serial.println("Nu s-a putut initializa RTC-ul!");
    sleep(100);
  }

  // Initializare TRMETRU
  trmetru.begin();
  trmetru.show();


  // 
  timestamp = rtc.now();
  Serial.print("/nInitialization done at: "); printTime();
  Serial.println("/n********LOOP Start********/n/n");
}

void loop() {
  timestamp = rtc.now();
  currentRPM = (millis() / 10) % MAX_RPM; // just cycles up and down for demo

  showRPM(currentRPM);
}

