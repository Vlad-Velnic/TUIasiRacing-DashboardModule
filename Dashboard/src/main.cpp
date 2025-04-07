#include "dashboard.h"


void setup() {
  Serial.begin(115200);
  Wire.begin();

  while (!rtc.begin()) {
    Serial.println("Nu s-a putut initializa RTC-ul!");
    sleep(100);
  }



  timestamp = rtc.now();
  Serial.print("Initialization done at: "); display_time();
  Serial.println("********LOOP Start********");
}

void loop() {
  timestamp = rtc.now();


}

