extern WebServer server;
extern float totalPathCM;

const int DRIVE_SPEED = 190;
const int TURN_SPEED = 150;
const float Kp = 2.5;
const int kEnc = 3;

// שכבת הבסיס למנועים: מקבלת 4 עוצמות PWM ושולחת לדרייבר
void driveMotors(int rFwd, int rBwd, int lFwd, int lBwd) {
  ledcWrite(rightForward, rFwd);
  ledcWrite(rightBackward, rBwd);
  ledcWrite(leftForward, lFwd);
  ledcWrite(leftBackward, lBwd);
}

// עצירה מוחלטת של כל המנועים
void stopMotors() {
  driveMotors(0, 0, 0, 0);
}

// אתחול ערוצי ה-PWM למנועים והגדרת פסיקות האנקודרים
void setupMotors() {
  ledcAttach(rightForward, 5000, 8);
  ledcAttach(rightBackward, 5000, 8);
  ledcAttach(leftForward, 5000, 8);
  ledcAttach(leftBackward, 5000, 8);

  pinMode(encoderLeftPin, INPUT_PULLUP);
  pinMode(encoderRightPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(encoderLeftPin), countLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderRightPin), countRightEncoder, RISING);

  stopMotors();
}

// נסיעה ישרה חכמה תיקון כפול לפי סטיית ג'ירו והפרש פולסים בין הגלגלים
void moveForwardSmart() {
  float gyroError = targetHeading - currentAngleZ;
  gyroError = constrain(gyroError, -30.0, 30.0);
  int gyroCorr = (int)(gyroError * Kp);

  int encError = (int)(pulseCountLeft - pulseCountRight);
  encError = constrain(encError, -20, 20);
  int encCorr = encError * kEnc;

  int correction = constrain(gyroCorr + encCorr, -60, 60);

  int speedL = constrain(DRIVE_SPEED - correction, 0, 255);
  int speedR = constrain(DRIVE_SPEED + correction, 0, 255);

  driveMotors(speedR, 0, speedL, 0);
}

// נסיעה בקשת ימינה לחיפוש הקיר כשהרובוט התרחק ממנו
void driftRight() {
  driveMotors(100, 0, DRIVE_SPEED, 0);
}

// פנייה שמאלה של 90 מעלות מבוקרת ג'ירו, עם הגנות timeout וזיהוי תקיעה
// שמאלה: spin (ימין קדימה, שמאל אחורה)
void dynamicTurnLeft() {
  printWallDimension();
  stopMotors();
  delay(150);

  updateRobotAngle();
  float target = currentAngleZ - 90.0;
  unsigned long t0 = millis();
  float lastAngle = currentAngleZ;
  int stuckCount = 0;

  driveMotors(TURN_SPEED, 0, 0, TURN_SPEED);

  while (abs(currentAngleZ - target) > 8.0) {
    updateRobotAngle();
    server.handleClient();

    if (abs(currentAngleZ - lastAngle) < 0.01) stuckCount++;
    else stuckCount = 0;
    lastAngle = currentAngleZ;

    if (stuckCount > 40) { Serial.println("GYRO FROZEN (L)"); break; }
    if (millis() - t0 > 4000) { Serial.println("L Timeout"); break; }
    delay(5);
  }

  stopMotors();
  delay(250);
  updateRobotAngle();
  targetHeading = currentAngleZ;
  headingAngle -= 90.0;
  totalPathCM += getDistanceTraveledCM();
  resetDistance();
}

// פנייה ימינה של 90 מעלות בשיטת pivot (גלגל שמאל בלבד), מבוקרת ג'ירו
void dynamicTurnRight() {
  printWallDimension();
  stopMotors();
  delay(150);

  updateRobotAngle();
  float target = currentAngleZ + 90.0;
  unsigned long t0 = millis();
  float lastAngle = currentAngleZ;
  int stuckCount = 0;

  driveMotors(0, 0, TURN_SPEED, 0);

  while (abs(currentAngleZ - target) > 8.0) {
    updateRobotAngle();
    server.handleClient();

    if (abs(currentAngleZ - lastAngle) < 0.01) stuckCount++;
    else stuckCount = 0;
    lastAngle = currentAngleZ;

    if (stuckCount > 40) { Serial.println("GYRO FROZEN (R)"); break; }
    if (millis() - t0 > 9000) { Serial.println("R Timeout"); break; }
    delay(5);
  }

  stopMotors();
  delay(250);
  updateRobotAngle();
  targetHeading = currentAngleZ;
  headingAngle += 90.0;
  totalPathCM += getDistanceTraveledCM();
  resetDistance();
}

// בחירת פעולה לפי מצב שני חיישני הקרבה
void runWallFollowerAlgorithm(int laserDistance) {
  if (!isScanning || isPaused) return;

  bool wallOnRight = (proxStateRight == WALL_DETECTED);
  bool wallInFront = (proxStateForward == WALL_DETECTED);

  if (wallInFront && wallOnRight) dynamicTurnLeft();
  else if (wallInFront && !wallOnRight) dynamicTurnRight();
  else if (!wallInFront && wallOnRight) moveForwardSmart();
  else driftRight();
}