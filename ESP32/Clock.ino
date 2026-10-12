// אתחול שעון הזמן-אמת; אם אבד המתח - כיוון אוטומטי לזמן ההידור
void setupClock() {
  if (!rtc.begin(&Wire)) {
    Serial.println("RTC not found!");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power - setting time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// מחזירה את השעה הנוכחית כמחרוזת בפורמט HH:MM
String getTimeString() {
  DateTime now = rtc.now();
  String hour = String(now.hour());
  String minute = String(now.minute());
  if (now.hour() < 10) hour = "0" + hour;
  if (now.minute() < 10) minute = "0" + minute;
  return hour + ":" + minute;
}