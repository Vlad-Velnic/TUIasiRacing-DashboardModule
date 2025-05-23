#include "dashboard.h"

const char *ssid = "iPhone";
const char *password = "coscos27";

void setup_wifi()
{
    delay(10);
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
}