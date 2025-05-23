#include "dashboard.h"

String pad(int number)
{
    return number < 10 ? "0" + String(number) : String(number);
}

void listSDFiles() {
  Serial.println("Files on SD card:");
  File root = SD.open("/");
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
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

void testSDWrite() {
  Serial.println("Testing SD write...");
  File testFile = SD.open("/write_test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Write test successful");
    testFile.printf("Free heap: %d\n", ESP.getFreeHeap());
    testFile.printf("Timestamp: %lu\n", millis());
    testFile.close();
    Serial.println("Write test completed");
  } else {
    Serial.println("Write test FAILED");
  }
}
