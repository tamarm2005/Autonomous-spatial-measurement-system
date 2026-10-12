#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <Adafruit_VL53L1X.h>
#include <RTClib.h>
#include <MPU6050_light.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <SD.h>

#define COLOR_DARK_BG tft.color565(20, 20, 30)
#define COLOR_GOLD tft.color565(255, 215, 0)

// --- מנועים ---
const int rightForward = 32;
const int rightBackward = 33;  
const int leftForward = 25;
const int leftBackward = 14;

// --- אנקודרים ---
const int encoderRightPin = 39;  
const int encoderLeftPin = 12;

volatile long pulseCountRight = 0;
volatile long pulseCountLeft = 0;

// --- חיישני קרבה (IR) ---
#define PROXIMITY_PIN_RIGHT 13
#define PROXIMITY_PIN_FORWARD 27
#define WALL_DETECTED HIGH

// --- מסך, טאץ', SD ---
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define TOUCH_CS_PIN 5
#define SD_CS_PIN 16

const int buzzerPin = 17;

XPT2046_Touchscreen ts(TOUCH_CS_PIN);
Adafruit_ST7796S tft = Adafruit_ST7796S(TFT_CS, TFT_DC, TFT_RST);
Adafruit_VL53L1X vl53 = Adafruit_VL53L1X();
RTC_DS3231 rtc;
MPU6050 mpu(Wire);

const char* ssid = "Robot_Measurer_WiFi";
const char* password = "123456789";
WebServer server(80);

// --- משתני מערכת ---
bool isScanning = false;
bool isPaused = false;

int proxStateRight = LOW;
int proxStateForward = LOW;

float currentAngleZ = 0.0;
float targetHeading = 0.0;

const int btnX = 80, btnY = 130, btnW = 320, btnH = 60;

// --- שרטוט ומידות ---
float roomX = 240.0, roomY = 120.0;
float headingAngle = 0.0;
float lastDistanceMoved = 0.0;
float startX = 240.0, startY = 120.0;
float wallStartX = 240.0, wallStartY = 120.0;
float currentWallLength = 0.0;
float totalPathCM = 0.0;
bool g_roomClosed = false;

#define MAX_PATH_POINTS 400
float pathX[MAX_PATH_POINTS];
float pathY[MAX_PATH_POINTS];
int pathIndex = 0;

// --- תוצאות סופיות ---
float g_roomHeightCM = 0.0;
float g_perimeterCM = 0.0;
float g_roomAreaM2 = 0.0;
float g_roomVolumeM3 = 0.0;
bool g_reportReady = false;
String g_reportTime = "";

extern float wallLengths[10];
extern int wallIndex;

// --- הצהרות ---
void setupDisplay();
void drawStartScreen();
void drawScanningUI();
void updateClockDisplay(String timeStr);
void updateMotionDisplay(float angle);
void updateDynamicDrawing(int laserDistance);
void printWallDimension();
void saveMapToSD();
void resetPath();
void addPathPoint(float x, float y);
void redrawPath();
void setupProximitySensor();
void readProximity();
void setupLaserSensor();
int readLaserDistance();
void setupClock();
String getTimeString();
void setupMotion();
void updateRobotAngle();
void setupGUI();
void runGUI();
void computeResultsToMemory();
void setupMotors();
void stopMotors();
void driftRight();
void runWallFollowerAlgorithm(int laserDistance);
void resetScanState();


// פסיקת חומרה: נקראת אוטומטית בכל פולס מהאנקודר הימני וסופרת אותו
void IRAM_ATTR countRightEncoder() {
  pulseCountRight++;
}

// פסיקת חומרה: נקראת אוטומטית בכל פולס מהאנקודר השמאלי וסופרת אותו
void IRAM_ATTR countLeftEncoder() {
  pulseCountLeft++;
}

// מאפסת את מוני האנקודרים לקראת מדידת קטע חדש
void resetDistance() {
  pulseCountRight = 0;
  pulseCountLeft = 0;
  lastDistanceMoved = 0.0;
}

// ממירה את פולסי האנקודרים למרחק בס"מ לפי קוטר הגלגל ורזולוציית האנקודר
float getDistanceTraveledCM() {
  const float WHEEL_DIAMETER_CM = 4.3;
  const float PULSES_PER_REVOLUTION = 210.0;
  long averagePulses = (pulseCountRight + pulseCountLeft) / 2;
  float circumference = WHEEL_DIAMETER_CM * 3.14159;
  return (averagePulses / PULSES_PER_REVOLUTION) * circumference;
}

// משמיעה צפצוף קצר לחיווי למשתמש
void beepBuzzer() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// מאפסת את כל נתוני הסריקה: מונים, זוויות, מיקום שרטוט ומערך קירות
void resetScanState() {
  resetDistance();
  currentAngleZ = 0.0;
  targetHeading = 0.0;
  roomX = 240.0;
  roomY = 120.0;
  startX = 240.0;
  startY = 120.0;
  wallStartX = 240.0;
  wallStartY = 120.0;
  headingAngle = 0.0;
  currentWallLength = 0.0;
  wallIndex = 0;
  g_reportReady = false;
  totalPathCM = 0.0;
  g_roomClosed = false;
  resetPath();
}

// ==========================================
//                   SETUP
// ==========================================
// אתחול חד-פעמי של כל רכיבי המערכת: SPI, מסך, חיישנים, ג'ירו ושרת WiFi
void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS_PIN, OUTPUT);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(19, INPUT_PULLUP);
  delay(50);

  SPI.begin(18, 19, 23);
  SD.begin(SD_CS_PIN, SPI, 1000000);
  digitalWrite(SD_CS_PIN, HIGH);

  setupMotors();

  ts.begin(SPI);
  ts.setRotation(1);

  setupDisplay();
  drawStartScreen();

  setupProximitySensor();

  Wire.begin(21, 22);
  Wire.setClock(100000);

  setupLaserSensor();
  setupClock();
  setupMotion();  // הרובוט חייב לעמוד בשקט בזמן הכיול!

  setupGUI();
  Serial.println("All systems ready.");
}

// ==========================================
//                    LOOP
// ==========================================
// הלולאה הראשית: מטפלת במגע, קוראת חיישנים ומריצה את אלגוריתם הניווט
void loop() {
  runGUI();

  static unsigned long lastTouchTime = 0;
  if (ts.touched() && millis() - lastTouchTime > 500) {
    lastTouchTime = millis();
    TS_Point p = ts.getPoint();

    if (p.z > 150) {
      int16_t touchX = constrain(map(p.x, 200, 3800, 0, 480), 0, 480);
      int16_t touchY = constrain(map(p.y, 3800, 200, 0, 320), 0, 320);

      // כפתור START
      if (!isScanning && touchX >= btnX && touchX <= btnX + btnW
          && touchY >= btnY && touchY <= btnY + btnH) {
        beepBuzzer();
        tft.fillScreen(COLOR_DARK_BG);
        drawScanningUI();
        delay(1500);
        resetScanState();
        isPaused = false;
        isScanning = true;
      } else if (isScanning) {
        // צד שמאל -> FINISH
        if (touchX >= 30 && touchX <= 210 && touchY >= 260 && touchY <= 310) {
          isScanning = false;
          stopMotors();
          if (currentWallLength > 5.0) printWallDimension();
          beepBuzzer();
          delay(100);
          beepBuzzer();

          tft.fillRect(0, 245, 480, 75, COLOR_DARK_BG);
          tft.setCursor(40, 265);
          tft.setTextColor(ST77XX_GREEN);
          tft.setTextSize(2);
          tft.print("SAVING REPORT...");

          computeResultsToMemory();
          saveMapToSD();
          delay(4000);
          setupDisplay();
          drawStartScreen();
        }
        // צד ימין -> PAUSE / RESUME
        else if (touchX >= 270 && touchX <= 450 && touchY >= 260 && touchY <= 310) {
          isPaused = !isPaused;
          stopMotors();
          tft.fillRoundRect(270, 260, 180, 50, 10,
                            isPaused ? ST77XX_GREEN : ST77XX_ORANGE);
          tft.setTextColor(ST77XX_BLACK);
          tft.setTextSize(2);
          tft.setCursor(325, 275);
          tft.print(isPaused ? "RESUME" : "PAUSE");
          beepBuzzer();
          delay(400);
        }
      }
    }
  }

  readProximity();
  int laserDistance = readLaserDistance();

  if (isScanning && !isPaused) {
    updateRobotAngle();
    updateMotionDisplay(currentAngleZ);
    updateDynamicDrawing(laserDistance);
    updateClockDisplay(getTimeString());
    runWallFollowerAlgorithm(laserDistance);
  } else {
    stopMotors();
  }

  delay(20);
}