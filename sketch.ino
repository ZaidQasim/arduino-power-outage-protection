#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

const int relayPin = 2;
const int buzzerPin = 8;
const int buttonPin = 7;

bool powerState = true;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

DateTime cutTimes[3];
int cutCount = 0;

unsigned long powerCutStartTime = 0;
bool waitingToRestore = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  digitalWrite(relayPin, HIGH);
  digitalWrite(buzzerPin, LOW);

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("System Ready for test");
}

void loop() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        powerState = !powerState;

        if (!powerState) {
          Serial.println("Electricity LOW");
          simulatePowerCut();
        } else {
          Serial.println("Electricity HIGH ");
          digitalWrite(relayPin, HIGH);
          digitalWrite(buzzerPin, LOW);
          waitingToRestore = false;
        }
      }
    }
  }

  lastButtonState = reading;

  if (waitingToRestore && (millis() - powerCutStartTime >= 900000)) {
    digitalWrite(relayPin, HIGH);
    powerState = true;
    waitingToRestore = false;
  }
}

void simulatePowerCut() {
  DateTime now = rtc.now();
  Serial.print("Electricity LOW In ");
  Serial.println(now.timestamp());

  if (cutCount < 3) {
    cutTimes[cutCount++] = now;
  } else {
    cutTimes[0] = cutTimes[1];
    cutTimes[1] = cutTimes[2];
    cutTimes[2] = now;
  }

  if (cutCount == 3) {
    TimeSpan diff = cutTimes[2] - cutTimes[0];
    Serial.print("Time between 1st and 3rd cut: ");
    Serial.print(diff.totalseconds());
    Serial.println(" seconds");

    if (diff.totalseconds() < 1800) {
      Serial.println("Three power outages within 30 minutes");
      digitalWrite(relayPin, LOW);
      tone(buzzerPin, 1000);
      delay(5000);
      noTone(buzzerPin);
      powerCutStartTime = millis();
      waitingToRestore = true;
    }
  }
}
