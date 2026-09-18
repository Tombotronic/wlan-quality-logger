// SD card test for M5Stack Cardputer Adv
// Mounts the microSD card (SPI: CS=G12, MOSI=G14, CLK=G40, MISO=G39),
// writes a test file, reads it back, and shows the result on screen.

#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS   12
#define SD_MOSI 14
#define SD_CLK  40
#define SD_MISO 39

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 0);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI)) {
    M5.Display.println("SD mount FAILED");
    return;
  }

  uint64_t sizeMB = SD.cardSize() / (1024 * 1024);
  M5.Display.printf("SD OK: %llu MB\n\n", sizeMB);

  File f = SD.open("/test.txt", FILE_WRITE);
  if (!f) {
    M5.Display.println("Write open FAILED");
    return;
  }
  f.println("Cardputer Adv SD test");
  f.close();

  f = SD.open("/test.txt", FILE_READ);
  if (!f) {
    M5.Display.println("Read open FAILED");
    return;
  }
  M5.Display.println("Read back:");
  while (f.available()) {
    M5.Display.write(f.read());
  }
  f.close();

  M5.Display.println("\nSD test PASSED");
}

void loop() {
  M5.update();
  delay(100);
}
