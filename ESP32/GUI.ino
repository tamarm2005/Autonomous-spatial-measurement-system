#include <WebServer.h>

// מעצבת ערך שטח לתצוגה מחזירה צורה לא תחומה אם החדר לא נסגר
String fmtArea(float v) {
  return (v < 0) ? "צורה לא תחומה" : String(v, 2) + " מ\"ר";
}

// מעצבת ערך נפח לתצוגה מחזירה צורה לא תחומה אם החדר לא נסגר
String fmtVolume(float v) {
  return (v < 0) ? "צורה לא תחומה" : String(v, 2) + " מ\"ק";
}

// החישובים הסופיים: גובה (ממוצע לייזר), היקף, שטח ונפח - נשמרים בזיכרון
void computeResultsToMemory() {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < 15; i++) {
    int d = readLaserDistance();
    if (d > 0) { sum += d; valid++; }
    delay(20);
  }
  g_roomHeightCM = (valid > 0) ? ((sum / valid) / 10.0) + 11.0 : 0.0;

  g_perimeterCM = totalPathCM + getDistanceTraveledCM();

  if (g_roomClosed) {
    float maxL = 0, maxW = 0;
    for (int i = 0; i < wallIndex; i++) {
      if (wallLengths[i] > maxL) {
        maxW = maxL;
        maxL = wallLengths[i];
      } else if (wallLengths[i] > maxW) {
        maxW = wallLengths[i];
      }
    }
    g_roomAreaM2 = (maxL / 100.0) * (maxW / 100.0);
    g_roomVolumeM3 = g_roomAreaM2 * (g_roomHeightCM / 100.0);
  } else {
    g_roomAreaM2 = -1.0;
    g_roomVolumeM3 = -1.0;
  }

  g_reportTime = getTimeString();
  g_reportReady = true;
}

// בניית דוח ה-HTML המלא להורדה מהאתר
String buildReportHTML() {
  String h = "<!DOCTYPE html><html dir='rtl' lang='he'><head><meta charset='utf-8'><title>דוח מדידת חלל</title>";
  h += "<style>body{font-family:'Segoe UI', Tahoma, Arial; background:#1a1a2e; color:#ffffff; text-align:center;}";
  h += ".container{background:#16213e; width:80%; margin:40px auto; padding:30px; border-radius:15px; box-shadow:0 0 20px rgba(0,0,0,0.5);}";
  h += "h1{color:#4ecca3;} table{width:80%; margin:20px auto; border-collapse:collapse;}";
  h += "th, td{border:1px solid #0f3460; padding:15px; font-size:18px;} th{background:#4ecca3; color:#1a1a2e;}</style></head>";
  h += "<body><div class='container'>";
  h += "<h1>דוח סיכום מדידת חלל אוטונומית</h1>";
  h += "<p><strong>תאריך ושעה: </strong>" + g_reportTime + "</p>";
  h += "<table><tr><th>פרמטר פיזי</th><th>תוצאה מחושבת</th></tr>";
  h += "<tr><td>גובה תקרה נטו (כולל קיזוז)</td><td>" + String(g_roomHeightCM, 2) + " ס\"מ</td></tr>";
  h += "<tr><td>היקף מסלול כולל</td><td>" + String(g_perimeterCM, 2) + " ס\"מ</td></tr>";
  h += "<tr><td>שטח חוסם משוער</td><td>" + fmtArea(g_roomAreaM2) + "</td></tr>";
  h += "<tr><td>נפח חלל מקסימלי</td><td>" + fmtVolume(g_roomVolumeM3) + "</td></tr>";
  h += "</table>";
  if (!g_roomClosed)
    h += "<p style='color:#ffb648;font-size:20px;'>* צורה לא תחומה - הרובוט לא חזר לנקודת ההתחלה</p>";

  h += "<h2>פירוט מידות קירות:</h2>";
  h += "<table><tr><th>מספר קיר</th><th>אורך</th></tr>";
  for (int i = 0; i < wallIndex; i++) {
    h += "<tr><td>קיר " + String(i + 1) + "</td><td>" + String(wallLengths[i], 2) + " ס\"מ</td></tr>";
  }
  h += "</table>";
  h += "</div></body></html>";
  return h;
}

// בניית דף הבית של האתר: כפתורים, שרטוט חי ב-canvas ודוח סיכום
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>מערכת מדידת חלל</title><style>";
  html += "body{font-family:'Segoe UI',Tahoma,sans-serif;text-align:center;background:#1a1a2e;color:#fff;direction:rtl;margin:0;padding:20px;}";
  html += ".container{background:#16213e;max-width:560px;margin:25px auto;padding:25px;border-radius:20px;box-shadow:0 10px 30px rgba(0,0,0,0.5);border:1px solid #0f3460;}";
  html += "h2{color:#4ecca3;margin-bottom:15px;}";
  html += ".btn{color:#1a1a2e;padding:14px 28px;font-size:18px;font-weight:bold;border:none;border-radius:50px;cursor:pointer;margin:6px;transition:transform .15s;}";
  html += ".btn:hover{transform:scale(1.05);}";
  html += ".green{background:linear-gradient(135deg,#4ecca3,#45b7af);}";
  html += ".orange{background:linear-gradient(135deg,#ffb648,#ff8c42);}";
  html += ".red{background:linear-gradient(135deg,#ff6b6b,#ee5253);color:#fff;}";
  html += ".status{margin:15px 0;padding:10px;background:rgba(255,255,255,0.05);border-radius:10px;}";
  html += "canvas{background:#0f3460;border-radius:12px;border:2px solid #4ecca3;width:100%;max-width:480px;height:auto;}";
  html += "table{width:100%;border-collapse:collapse;margin-top:15px;}";
  html += "th,td{border:1px solid #0f3460;padding:10px;font-size:16px;}th{background:#4ecca3;color:#1a1a2e;}";
  html += ".hide{display:none;}";
  html += "</style></head><body><div class='container'>";

  html += "<h2>מערכת מדידת חלל אוטונומית</h2>";
  html += "<div class='status'>סטטוס: <strong id='st'>ממתין לפקודה</strong></div>";

  html += "<div id='ctl'>";
  html += "<button class='btn green' id='bStart' onclick='act(\"/start\")'>הפעלה</button>";
  html += "<button class='btn orange hide' id='bPause' onclick='togglePause()'>עצירה</button>";
  html += "<button class='btn red hide' id='bStop' onclick='act(\"/finish\")'>סיום</button>";
  html += "</div>";

  html += "<div id='board' class='hide'><h3 style='color:#4ecca3;'>שרטוט בזמן אמת</h3>";
  html += "<canvas id='c' width='480' height='240'></canvas></div>";

  html += "<div id='rep' class='hide'></div>";
  html += "</div>";

  html += "<script>";
  html += "var paused=false, finished=false;";
  html += "var cv=document.getElementById('c'), ct=cv.getContext('2d');";

  html += "function act(u){fetch(u).then(function(){tick();});}";
  html += "function togglePause(){act(paused?'/resume':'/pause');}";

  html += "function ui(d){";
  html += " document.getElementById('bStart').className='btn green'+((d.scan||d.ready)?' hide':'');";
  html += " document.getElementById('bPause').className='btn orange'+(d.scan?'':' hide');";
  html += " document.getElementById('bStop').className='btn red'+(d.scan?'':' hide');";
  html += " document.getElementById('bPause').innerText=d.pause?'המשך':'עצירה';";
  html += " if(d.scan||d.n>1) document.getElementById('board').className='';";
  html += " if(d.scan){document.getElementById('st').innerText=d.pause?'מושהה - ניתן להמשיך':'הרובוט במדידה...';}";
  html += " else if(d.ready){document.getElementById('st').innerText='המדידה הושלמה';}";
  html += " else {document.getElementById('st').innerText='ממתין לפקודה';}";
  html += " paused=(d.pause==1);";
  html += "}";

  html += "function draw(d){";
  html += " ct.clearRect(0,0,480,240);";
  html += " if(d.n>0){";
  html += "  ct.strokeStyle='#00e5ff';ct.lineWidth=3;ct.lineJoin='round';";
  html += "  ct.beginPath();ct.moveTo(d.p[0],d.p[1]);";
  html += "  for(var i=1;i<d.n;i++){ct.lineTo(d.p[i*2],d.p[i*2+1]);}";
  html += "  ct.stroke();";
  html += "  ct.fillStyle='#4ecca3';ct.beginPath();ct.arc(d.p[0],d.p[1],5,0,6.283);ct.fill();";
  html += "  ct.fillStyle='#ff5252';ct.beginPath();ct.arc(d.p[(d.n-1)*2],d.p[(d.n-1)*2+1],5,0,6.283);ct.fill();";
  html += " }";
  html += "}";

  html += "function showReport(){";
  html += " if(finished)return; finished=true;";
  html += " fetch('/summary').then(function(r){return r.json();}).then(function(s){";
  html += "  var t=\"<h3 style='color:#4ecca3;'>דוח סיכום</h3><p>תאריך ושעה: \"+s.time+\"</p>\";";
  html += "  t+=\"<table><tr><th>פרמטר פיזי</th><th>תוצאה מחושבת</th></tr>\";";
  html += "  t+=\"<tr><td>גובה תקרה נטו</td><td>\"+s.h+\" ס\\\"מ</td></tr>\";";
  html += "  t+=\"<tr><td>היקף מסלול כולל</td><td>\"+s.per+\" ס\\\"מ</td></tr>\";";
  html += "  t+=\"<tr><td>שטח חוסם משוער</td><td>\"+s.area+\"</td></tr>\";";
  html += "  t+=\"<tr><td>נפח חלל מקסימלי</td><td>\"+s.vol+\"</td></tr></table>\";";
  html += "  t+=\"<table><tr><th>מספר קיר</th><th>אורך</th></tr>\";";
  html += "  for(var i=0;i<s.w.length;i++){t+=\"<tr><td>קיר \"+(i+1)+\"</td><td>\"+s.w[i]+\" ס\\\"מ</td></tr>\";}";
  html += "  t+=\"</table>\";";
  html += "  t+=\"<a class='btn green' style='text-decoration:none;display:inline-block;' href='/download'>הורדת הדוח</a>\";";
  html += "  t+=\" <button class='btn orange' onclick='location.reload()'>מדידה חדשה</button>\";";
  html += "  var r=document.getElementById('rep'); r.innerHTML=t; r.className='';";
  html += " });";
  html += "}";

  html += "function tick(){";
  html += " fetch('/data').then(function(r){return r.json();}).then(function(d){";
  html += "  ui(d); draw(d);";
  html += "  if(d.scan==0 && d.ready==1) showReport();";
  html += " }).catch(function(e){});";
  html += "}";
  html += "setInterval(tick,500); tick();";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

// API: מחזירה JSON עם מצב הסריקה וכל נקודות המסלול לציור באתר
void handleData() {
  String json = "{\"scan\":";
  json += (isScanning ? "1" : "0");
  json += ",\"pause\":";
  json += (isPaused ? "1" : "0");
  json += ",\"ready\":";
  json += (g_reportReady ? "1" : "0");
  json += ",\"n\":";
  json += String(pathIndex);
  json += ",\"p\":[";
  for (int i = 0; i < pathIndex; i++) {
    if (i > 0) json += ",";
    json += String((int)pathX[i]);
    json += ",";
    json += String((int)pathY[i]);
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// API: מחזירה JSON עם תוצאות המדידה הסופיות
void handleSummary() {
  String j = "{\"time\":\"" + g_reportTime + "\"";
  j += ",\"h\":\"" + String(g_roomHeightCM, 2) + "\"";
  j += ",\"per\":\"" + String(g_perimeterCM, 2) + "\"";
  j += ",\"area\":\"" + fmtArea(g_roomAreaM2) + "\"";
  j += ",\"vol\":\"" + fmtVolume(g_roomVolumeM3) + "\"";
  j += ",\"w\":[";
  for (int i = 0; i < wallIndex; i++) {
    if (i > 0) j += ",";
    j += "\"" + String(wallLengths[i], 2) + "\"";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

// טיפול בלחיצת "הפעלה" מהאתר: איפוס והתחלת סריקה
void handleStart() {
  Serial.println("WEB: START");
  tft.fillScreen(COLOR_DARK_BG);
  drawScanningUI();
  resetScanState();
  isPaused = false;
  isScanning = true;
  server.send(200, "text/plain", "started");
}

// טיפול בלחיצת "עצירה" מהאתר: השהיית הסריקה ועצירת מנועים
void handlePause() {
  if (isScanning) {
    isPaused = true;
    stopMotors();
  }
  server.send(200, "text/plain", "paused");
}

// טיפול בלחיצת "המשך" מהאתר: חזרה לסריקה מאותה נקודה
void handleResume() {
  if (isScanning) isPaused = false;
  server.send(200, "text/plain", "resumed");
}

// טיפול בלחיצת "סיום" מהאתר: עצירה, חישוב תוצאות והצגת הדוח
void handleFinish() {
  isScanning = false;
  isPaused = false;
  stopMotors();

  if (currentWallLength > 5.0) printWallDimension();

  computeResultsToMemory();

  setupDisplay();
  drawStartScreen();

  Serial.println("WEB: FINISH - report ready");
  server.send(200, "text/plain", "finished");
}

// הצגת הדוח המלא בדפדפן
void handleReport() {
  if (!g_reportReady) {
    server.send(200, "text/html", "<meta charset='utf-8'><h2>אין דוח זמין עדיין</h2>");
    return;
  }
  server.send(200, "text/html", buildReportHTML());
}

// הורדת הדוח כקובץ HTML למחשב/טלפון
void handleDownload() {
  if (!g_reportReady) {
    server.send(200, "text/html", "<meta charset='utf-8'><h2>אין דוח זמין עדיין</h2>");
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=room_report.html");
  server.send(200, "text/html", buildReportHTML());
}

// הקמת רשת ה-WiFi של הרובוט ורישום כל נתיבי השרת
void setupGUI() {
  WiFi.softAP(ssid, password);
  Serial.print("GUI IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/summary", handleSummary);
  server.on("/start", handleStart);
  server.on("/pause", handlePause);
  server.on("/resume", handleResume);
  server.on("/finish", handleFinish);
  server.on("/report", handleReport);
  server.on("/download", handleDownload);
  server.begin();
}

// נקראת בכל לולאה: מטפלת בבקשות נכנסות מהדפדפן
void runGUI() {
  server.handleClient();
}