float wallLengths[10];
int wallIndex = 0;
float drawScale = 2.5;

// מוסיפה נקודה למערך המסלול שנשלח לשרטוט באתר
void addPathPoint(float x, float y) {
  if (pathIndex < MAX_PATH_POINTS) {
    pathX[pathIndex] = x;
    pathY[pathIndex] = y;
    pathIndex++;
  }
}

// מאפסת את מערך המסלול ומציבה את נקודת ההתחלה
void resetPath() {
  pathIndex = 0;
  addPathPoint(roomX, roomY);
}

// אתחול המסך וציור מסך הפתיחה עם כותרת המערכת
void setupDisplay() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_DARK_BG);
  tft.drawRect(5, 5, 470, 310, COLOR_GOLD);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(3);
  tft.setCursor(40, 40);
  tft.print("Autonomous Room Mapper");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(140, 80);
  tft.print("System Ready.");
}

// ציור כפתור START SCAN הירוק במסך הפתיחה
void drawStartScreen() {
  tft.fillRoundRect(80, 130, 320, 60, 10, ST77XX_GREEN);
  tft.drawRoundRect(80, 130, 320, 60, 10, ST77XX_WHITE);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setCursor(150, 148);
  tft.print("START SCAN");
}

// ציור ממשק הסריקה: כפתורי FINISH ו-PAUSE בתחתית המסך
void drawScanningUI() {
  tft.drawFastHLine(0, 245, 480, ST77XX_WHITE);

  tft.fillRoundRect(30, 260, 180, 50, 10, ST77XX_RED);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(90, 275);
  tft.print("FINISH");

  tft.fillRoundRect(270, 260, 180, 50, 10, isPaused ? ST77XX_GREEN : ST77XX_ORANGE);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(325, 275);
  tft.print(isPaused ? "RESUME" : "PAUSE");
}

// עדכון תצוגת השעה בראש המסך
void updateClockDisplay(String timeStr) {
  tft.fillRect(190, 10, 100, 30, COLOR_DARK_BG);
  tft.setCursor(190, 10);
  tft.setTextColor(COLOR_GOLD);
  tft.setTextSize(2);
  tft.print(timeStr);
}

// עדכון תצוגת זווית הג'ירו הנוכחית על המסך
void updateMotionDisplay(float angle) {
  tft.fillRect(20, 50, 180, 30, COLOR_DARK_BG);
  tft.setCursor(20, 50);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.print("Angle: ");
  tft.print(angle, 1);
}

// שמירת דוח HTML מלא לכרטיס ה-SD, כולל ניהול שיתוף אפיק ה-SPI
void saveMapToSD() {
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  delay(50);
  SD.end();
  if (!SD.begin(SD_CS_PIN, SPI, 1000000)) {
    digitalWrite(TFT_CS, LOW);
    tft.fillRect(0, 245, 480, 75, COLOR_DARK_BG);
    tft.setCursor(30, 265);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.print("SD NOT DETECTED");
    return;
  }

  if (SD.exists("/rep.htm")) {
    SD.remove("/rep.htm");
  }

  File file = SD.open("/rep.htm", FILE_WRITE);
  if (!file) {
    digitalWrite(SD_CS_PIN, HIGH);
    digitalWrite(TFT_CS, LOW);
    tft.fillRect(0, 245, 480, 75, COLOR_DARK_BG);
    tft.setCursor(40, 265);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.print("ERROR: CANT WRITE TO SD");
    return;
  }

  file.println("<!DOCTYPE html><html dir='rtl' lang='he'><head><meta charset='utf-8'><title>דוח מדידת חלל</title>");
  file.println("<style>body{font-family:'Segoe UI', Tahoma, Arial; background:#1a1a2e; color:#ffffff; text-align:center;}");
  file.println(".container{background:#16213e; width:80%; margin:40px auto; padding:30px; border-radius:15px; box-shadow:0 0 20px rgba(0,0,0,0.5);}");
  file.println("h1{color:#4ecca3;} table{width:80%; margin:20px auto; border-collapse:collapse;}");
  file.println("th, td{border:1px solid #0f3460; padding:15px; font-size:18px;} th{background:#4ecca3; color:#1a1a2e;}</style></head>");
  file.println("<body><div class='container'>");
  file.println("<h1>דוח סיכום מדידת חלל אוטונומית</h1>");
  file.print("<p><strong>תאריך ושעה: </strong>");
  file.print(getTimeString());
  file.println("</p>");

  file.println("<table><tr><th>פרמטר פיזי</th><th>תוצאה מחושבת</th></tr>");
  file.print("<tr><td>גובה תקרה נטו (כולל קיזוז)</td><td>");
  file.print(g_roomHeightCM);
  file.println(" ס\"מ</td></tr>");
  file.print("<tr><td>היקף מסלול כולל</td><td>");
  file.print(g_perimeterCM);
  file.println(" ס\"מ</td></tr>");
  file.print("<tr><td>שטח חוסם משוער</td><td>");
  file.print(g_roomAreaM2 < 0 ? String("צורה לא תחומה") : String(g_roomAreaM2, 2) + " מ\"ר");
  file.println("</td></tr>");
  file.print("<tr><td>נפח חלל מקסימלי</td><td>");
  file.print(g_roomVolumeM3 < 0 ? String("צורה לא תחומה") : String(g_roomVolumeM3, 2) + " מ\"ק");
  file.println("</td></tr>");
  file.println("</table>");

  file.println("<h2>פירוט מידות קירות:</h2><ul style='list-style:none; padding:0; font-size:18px;'>");
  for (int i = 0; i < wallIndex; i++) {
    file.print("<li style='background:#0f3460; margin:10px auto; width:50%; padding:10px; border-radius:8px;'>קיר ");
    file.print(i + 1);
    file.print(": <strong>");
    file.print(wallLengths[i]);
    file.println(" ס\"מ</strong></li>");
  }
  file.println("</ul></div></body></html>");

  file.flush();
  file.close();

  digitalWrite(SD_CS_PIN, HIGH);
  delay(10);
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TOUCH_CS_PIN, LOW);

  tft.fillRect(0, 245, 480, 75, COLOR_DARK_BG);
  tft.setCursor(30, 260);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println("SUCCESS! FILE SAVED.");
  tft.setCursor(30, 285);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("YOU CAN REMOVE SD CARD.");

  digitalWrite(buzzerPin, HIGH);
  delay(1500);
  digitalWrite(buzzerPin, LOW);
}

// שמירת אורך הקיר שהסתיים במערך, כתיבתו על המסך ואיפוס לקיר הבא
void printWallDimension() {
  float realWallLength = currentWallLength + 2.0;
  if (wallIndex < 10 && realWallLength > 5.0) {
    wallLengths[wallIndex] = realWallLength;
    wallIndex++;
  }

  int midX = (wallStartX + roomX) / 2;
  int midY = (wallStartY + roomY) / 2;

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(midX - 15, midY - 15);
  tft.print((int)currentWallLength);

  wallStartX = roomX;
  wallStartY = roomY;
  currentWallLength = 0.0;
}

// השרטוט החי: המרת מרחק וזווית לקו על המסך, כולל זיהוי סגירת מעגל
void updateDynamicDrawing(int laserDistance) {
  float currentDist = getDistanceTraveledCM();
  float dDist = currentDist - lastDistanceMoved;
  if (abs(dDist) <= 0.5) return;

  currentWallLength += abs(dDist);

  float rad = currentAngleZ * 3.14159 / 180.0;
  float nextX = roomX + (dDist * sin(rad) * drawScale);
  float nextY = roomY - (dDist * cos(rad) * drawScale);

  if (nextX < 15 || nextX > 465 || nextY < 15 || nextY > 230) {
    drawScale *= 0.7;
    redrawPath();
    lastDistanceMoved = currentDist;
    return;
  }

  tft.drawLine((int)roomX, (int)roomY, (int)nextX, (int)nextY, ST77XX_CYAN);
  roomX = nextX;
  roomY = nextY;
  addPathPoint(roomX, roomY);
  lastDistanceMoved = currentDist;

  float traveled = totalPathCM + currentDist;
  if (traveled > 50.0 && abs(roomX - startX) < 25.0 && abs(roomY - startY) < 25.0) {
    stopMotors();
    isScanning = false;
    g_roomClosed = true;
    printWallDimension();

    beepBuzzer();
    delay(150);
    beepBuzzer();
    delay(150);
    beepBuzzer();

    tft.fillRect(0, 245, 480, 75, COLOR_DARK_BG);
    tft.setCursor(40, 265);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.print("ROOM CLOSED! SAVING...");

    computeResultsToMemory();
    saveMapToSD();
    delay(4000);
    setupDisplay();
    drawStartScreen();
  }
}

// ציור מחדש של כל המסלול בקנה מידה מוקטן כשהשרטוט חורג מגבולות המסך
void redrawPath() {
  tft.fillRect(10, 10, 460, 225, COLOR_DARK_BG);
  if (pathIndex < 2) return;

  float minX = pathX[0], maxX = pathX[0], minY = pathY[0], maxY = pathY[0];
  for (int i = 1; i < pathIndex; i++) {
    if (pathX[i] < minX) minX = pathX[i];
    if (pathX[i] > maxX) maxX = pathX[i];
    if (pathY[i] < minY) minY = pathY[i];
    if (pathY[i] > maxY) maxY = pathY[i];
  }
  float cx = (minX + maxX) / 2.0, cy = (minY + maxY) / 2.0;
  float dx = 240.0 - cx, dy = 120.0 - cy;

  for (int i = 0; i < pathIndex; i++) {
    pathX[i] = (pathX[i] - cx) * 0.7 + cx + dx;
    pathY[i] = (pathY[i] - cy) * 0.7 + cy + dy;
  }
  startX = (startX - cx) * 0.7 + cx + dx;
  startY = (startY - cy) * 0.7 + cy + dy;

  for (int i = 1; i < pathIndex; i++)
    tft.drawLine((int)pathX[i - 1], (int)pathY[i - 1], (int)pathX[i], (int)pathY[i], ST77XX_CYAN);

  roomX = pathX[pathIndex - 1];
  roomY = pathY[pathIndex - 1];
}