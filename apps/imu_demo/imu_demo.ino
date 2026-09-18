// IMU demo for M5Stack Cardputer Adv
// Shows live accelerometer + gyro values from the onboard BMI270.
//
// Library needed: M5Unified (Arduino Library Manager)

#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  if (M5.Imu.getType() == m5::imu_none) {
    M5.Display.println("No IMU found!");
    while (true) { delay(1000); }
  }
}

void loop() {
  if (M5.Imu.update()) {
    auto data = M5.Imu.getImuData();

    M5.Display.setCursor(0, 0);
    M5.Display.printf("ACCEL (g)\n");
    M5.Display.printf(" x:%6.2f\n", data.accel.x);
    M5.Display.printf(" y:%6.2f\n", data.accel.y);
    M5.Display.printf(" z:%6.2f\n", data.accel.z);
    M5.Display.printf("\n");
    M5.Display.printf("GYRO (dps)\n");
    M5.Display.printf(" x:%6.1f\n", data.gyro.x);
    M5.Display.printf(" y:%6.1f\n", data.gyro.y);
    M5.Display.printf(" z:%6.1f\n", data.gyro.z);
  }

  M5.update();
  delay(50);
}
