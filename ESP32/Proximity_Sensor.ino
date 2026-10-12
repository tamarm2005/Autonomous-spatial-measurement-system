// הגדרת פיני חיישני הקרבה ככניסות
void setupProximitySensor() {
  pinMode(PROXIMITY_PIN_RIGHT, INPUT);
  pinMode(PROXIMITY_PIN_FORWARD, INPUT);
}

// קריאת מצב שני חיישני הקרבה (קדמי וימני) לעדכון האלגוריתם
void readProximity() {
  proxStateRight = digitalRead(PROXIMITY_PIN_RIGHT);
  proxStateForward = digitalRead(PROXIMITY_PIN_FORWARD);
}