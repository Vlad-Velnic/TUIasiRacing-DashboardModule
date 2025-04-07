#include "dashboard.h"

void display_time()
{
    Serial.print(timestamp.hour());
    Serial.print(':');
    Serial.print(timestamp.minute());
    Serial.print(':');
    Serial.print(timestamp.second());
}