// אתחול חיישן הלייזר ToF בכתובת 0x29 והפעלת מדידה רציפה
void setupLaserSensor() {
  if (vl53.begin(0x29, &Wire)) {
    vl53.setTimingBudget(50);
    vl53.startRanging();
    Serial.println("Laser VL53L1X found.");
  } else {
    Serial.println("Laser VL53L1X NOT found!");
  }
}

// קריאת מרחק בודדת מהלייזר במ"מ; מחזירה 2- אם אין נתון מוכן
int readLaserDistance() {
  if (vl53.dataReady()) {
    int distance = vl53.distance();
    vl53.clearInterrupt();
    return distance;
  }
  return -2;
}

// מדידת גובה תקרה מדויקת: ממוצע 5 קריאות, מוחזר בס"מ
float measureCeilingHeight() {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < 5; i++) {
    int d = readLaserDistance();
    if (d > 0) { sum += d; valid++; }
    delay(50);
  }
  return (valid > 0) ? (sum / valid) / 10.0 : 0.0;
}