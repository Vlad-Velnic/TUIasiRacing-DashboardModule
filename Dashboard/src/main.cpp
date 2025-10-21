// 172.20.10.3 pe iPhone


#include "dashboard.h"


void setup() {


   // Initialize the dashboard
   if (!dashboard.initialize()) {
       // If initialization fails, blink an error pattern
       while (true) {
           digitalWrite(LED_BUILTIN, HIGH);
           delay(100);
           digitalWrite(LED_BUILTIN, LOW);
           delay(100);
       }
   }


}


void loop() {
   // Update the dashboard
   dashboard.update();
}
