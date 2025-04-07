#include "dashboard.h"

void showRPM(int currentRPM) {
    int ledsToLight = map(currentRPM, MIN_RPM, MAX_RPM, 0, NUM_LEDS);
  
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < ledsToLight) {
        // Color based on position: green → yellow → red
        uint32_t color;

        if (i <= NUM_LEDS * 0.5) {

          color = trmetru.Color(0, 255, 0); // Green

        } else if (i <= NUM_LEDS * 0.8) {

          color = trmetru.Color(255, 165, 0); // Orange

        } else {
            
          color = trmetru.Color(255, 0, 0); // Red

        }
        trmetru.setPixelColor(i, color);

      } else {

        trmetru.setPixelColor(i, 0); // Turn off

      }
    }
  
    trmetru.show();
  }