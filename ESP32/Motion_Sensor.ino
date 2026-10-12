// אתחול הג'ירוסקופ בכתובת 0x69 וביצוע כיול אוטומטי (הרובוט חייב לעמוד)
void setupMotion() {
  mpu.setAddress(0x69);

  byte status = mpu.begin();
  if (status != 0) {
    Serial.print("MPU6050 FAILED, status=");
    Serial.println(status);
    tft.setCursor(40, 200);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.print("IMU ERROR!");
    return;
  }

  Serial.println("Calibrating gyro - DO NOT MOVE!");
  delay(1000);
  mpu.calcOffsets();
  Serial.println("Gyro calibrated.");
  currentAngleZ = 0.0;
}

// קריאת הזווית העדכנית מהג'ירו ועדכון משתנה הזווית הגלובלי
void updateRobotAngle() {
  mpu.update();
  currentAngleZ = mpu.getAngleZ();
}