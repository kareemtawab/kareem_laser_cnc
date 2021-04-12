#include <Timer.h>
#include <Encoder.h>

#define EMB 12
#define IN1 11
#define IN2 10
#define IN3 9
#define IN4 8
#define IN5 7
#define IN6 6

boolean isOn = false;
boolean firstrelaystate = false;
char receivedChar;
String serialstring;

Timer t;
Encoder myEnc(2, 3);

void setup()
{
  pinMode(13, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT);
  pinMode(IN6, OUTPUT);
  pinMode(EMB, INPUT_PULLUP);
  pinMode(EMB, OUTPUT);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, LOW);
  t.oscillate(13, 500, HIGH);
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  Serial.begin(115200);
}

void loop() {
  if (digitalRead(EMB) == HIGH) {
    if (Serial.available() > 0) {
      receivedChar = Serial.read();
      if (receivedChar == '<') {
        serialstring = Serial.readStringUntil('>');
        Serial.flush();
      }

      if (serialstring.startsWith("n", 0)) {
        isOn = true;
      }

      if (serialstring.startsWith("f", 0)) {
        isOn = false;
      }
    }

    if (isOn && serialstring.startsWith("1", 1)) {
      digitalWrite(IN1, LOW);
      firstrelaystate = true;
      //Serial.println("First relay is now ON");
    }

    if (isOn && serialstring.startsWith("2", 1)) {
      digitalWrite(IN2, LOW);
      //Serial.println("Second relay is now ON");
    }

    if (isOn && serialstring.startsWith("3", 1)) {
      digitalWrite(IN3, LOW);
      //Serial.println("Third relay is now ON");
    }

    if (isOn && serialstring.startsWith("4", 1)) {
      digitalWrite(IN4, LOW);
      //Serial.println("Fourth relay is now ON");
    }

    if (isOn && serialstring.startsWith("5", 1)) {
      digitalWrite(IN5, HIGH);
      //Serial.println("Fifth relay is now ON");
    }

    if (isOn && serialstring.startsWith("6", 1)) {
      digitalWrite(IN6, HIGH);
      //Serial.println("Sixth relay is now ON");
    }

    if (!isOn && serialstring.startsWith("1", 1)) {
      digitalWrite(IN1, HIGH);
      firstrelaystate = false;
      digitalWrite(13, LOW);
      //Serial.println("First relay is now OFF");
    }

    if (!isOn && serialstring.startsWith("2", 1)) {
      digitalWrite(IN2, HIGH);
      //Serial.println("Second relay is now OFF");
    }

    if (!isOn && serialstring.startsWith("3", 1)) {
      digitalWrite(IN3, HIGH);
      //Serial.println("Third relay is now OFF");
    }

    if (!isOn && serialstring.startsWith("4", 1)) {
      digitalWrite(IN4, HIGH);
      //Serial.println("Fourth relay is now OFF");
    }

    if (!isOn && serialstring.startsWith("5", 1)) {
      digitalWrite(IN5, LOW);
      //Serial.println("Fifth relay is now OFF");
    }

    if (!isOn && serialstring.startsWith("6", 1)) {
      digitalWrite(IN6, LOW);
      //Serial.println("Sixth relay is now OFF");
    }

    if (firstrelaystate) {
      t.update();
    }

    long oldPosition  = -999;

    long newPosition = myEnc.read() / 4;
    if (newPosition != oldPosition) {
      oldPosition = newPosition;
      Serial.println(newPosition);
    }
  }
  else {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, HIGH);
    digitalWrite(IN5, LOW);
    digitalWrite(IN6, LOW);
    digitalWrite(13, HIGH);
    firstrelaystate = false;
    while (1) {}
  }
}

