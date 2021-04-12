#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <EEPROM.h>

time_t elapsedTime;

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 11, 10, 9, 8);

#define SplittingArraySize 12
String SplittingArray[SplittingArraySize];
String state;
String dirty_status;
String clean_status;
String X_MPOS;
String Y_MPOS;
String Z_MPOS;
String X_WPOS;
String Y_WPOS;
String Z_WPOS;
String BUF;

float hrs; // hours of laser life
const int recordminutes = 1;
const int recorderarraysize = 60 / recordminutes;
float recordat[recorderarraysize];
const float m = (float)recordminutes / 60;

// Data wire is plugged into pin 12 on the Arduino
#define ONE_WIRE_BUS 7
#define NUB 0 // NUBM44 temp sensor index
#define SXD 1 // SXD temp sensor index

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int t1; // temp of NUBM44
int t2; // temp of SXD

#define BL 6 // Backlight pin
#define BUZZER 13 // Buzzer pin

void setup() {

  pinMode(BL, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  analogWrite(BL, 200);
  digitalWrite(BUZZER, LOW);
  Serial.begin(115200);
  Serial.println("Serial Monitor Connected");
  u8g2.begin();
  sensors.begin();  // Start up the library
  sensors.setResolution(9); // set resolution of temp bits (from 9 to 12)
  Serial.print("System will record LASER life to EEPROM in hours every ");
  Serial.print(recordminutes);
  Serial.print(" min(s), i.e ");
  Serial.print(recorderarraysize);
  Serial.println(" times per hour at these hour portions:");

  for (int x = 0; x <= recorderarraysize - 1; x++) {
    recordat[x] = m * (x + 1);
    Serial.println(recordat[x], 3);
  }

}

void loop() {

  sensors.requestTemperatures();
  t1 = sensors.getTempCByIndex(0);
  t2 = sensors.getTempCByIndex(1);

  state = "-N/A-";
  elapsedTime = now();

  do {
    Serial.flush();
    //delay (1);
    if (Serial.read() == '<') {              //If Serial.read() is a <
      Serial.flush();
      Serial.setTimeout(500);
      dirty_status = Serial.readStringUntil('>');
      int startofstatus = dirty_status.indexOf("<");
      int endofstatus = dirty_status.indexOf(">");
      clean_status = dirty_status.substring(startofstatus + 1, endofstatus - 0);
      clean_status.toUpperCase();
      clean_status.replace("MPOS:", "");
      clean_status.replace("WPOS:", "");
      clean_status.replace("BUF:", "");
      Split(clean_status, ",");
      state = SplittingArray[0];
      X_MPOS = SplittingArray[1];
      Y_MPOS = SplittingArray[2];
      Z_MPOS = SplittingArray[3];
      X_WPOS = SplittingArray[4];
      Y_WPOS = SplittingArray[5];
      Z_WPOS = SplittingArray[6];
      BUF = SplittingArray[7];
      Serial.println(clean_status);
      //delay (5);
      Serial.flush();

    }
  } while (Serial.available());

  hrs = millis() / (3600.000 * 1000.000);
  int a;
  float b;
  a = floor(hrs);
  b = ((hrs - a) * pow(10, 3)) / 1000;

  for (int i = 0; i <= recorderarraysize - 1; i++) {
    if (b == recordat[i]) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      Serial.print("Recorded hours at ");
      Serial.print(i + 1);
      Serial.print(" from ");
      Serial.print(recorderarraysize);
      Serial.println(" min(s)!");
      break;
    }
  }

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_saikyosansbold8_8u);
    u8g2.drawStr(37, 8, "GRBL");
    u8g2.setFont(u8g2_font_chikita_tr);
    u8g2.drawStr(70, 7, "v0.9j");
    u8g2.setFont(u8g2_font_micro_mr);
    u8g2.drawStr(49, 15, "RUN TIME");
    u8g2.drawStr(45, 30, "LASER LIFE");
    u8g2.drawStr(6, 32, "NUBM44");
    u8g2.drawStr(9, 38, "TEMP");
    u8g2.drawStr(98, 32, "SXDRIVE");
    u8g2.drawStr(104, 38, "TEMP");
    u8g2.drawVLine(33, 2, 35);
    u8g2.drawVLine(94, 2, 35);

    u8g2.setFont(u8g2_font_profont10_mr);
    u8g2.drawStr(53, 22, ":");
    u8g2.drawStr(71, 22, ":");
    if (hour(elapsedTime) >= 10) {
      u8g2.setCursor(42, 23);
      u8g2.print(hour(elapsedTime));
    }
    else {
      u8g2.drawStr(42, 23, "0");
      u8g2.setCursor(47, 23);
      u8g2.print(hour(elapsedTime));

    }
    if (minute(elapsedTime) >= 10) {
      u8g2.setCursor(60, 23);
      u8g2.print(minute(elapsedTime));
    }
    else {
      u8g2.drawStr(60, 23, "0");
      u8g2.setCursor(65, 23);
      u8g2.print(minute(elapsedTime));
    }
    if (second(elapsedTime) >= 10) {
      u8g2.setCursor(79, 23);
      u8g2.print(second(elapsedTime));
    }
    else {
      u8g2.drawStr(79, 23, "0");
      u8g2.setCursor(84, 23);
      u8g2.print(second(elapsedTime));
    }

    u8g2.setCursor(40, 38);
    u8g2.print(hrs, 3);
    u8g2.drawStr(75, 38, "hrs");

    u8g2.setFont(u8g2_font_logisoso24_tn);
    u8g2.setCursor(0, 25);
    u8g2.print(t1);
    u8g2.setCursor(96, 25);
    u8g2.print(t2);

    u8g2.setFont(u8g2_font_chikita_tr);
    u8g2.drawStr(37, 48, "MX=");
    u8g2.drawStr(37, 55, "MY=");
    u8g2.drawStr(37, 62, "MZ=");
    u8g2.drawVLine(81, 44, 16);
    u8g2.drawStr(84, 48, "WX=");
    u8g2.drawStr(84, 55, "WY=");
    u8g2.drawStr(84, 62, "WZ=");
    u8g2.setCursor(3, 60);
    u8g2.print(state);
    //u8g2.setFont(u8g2_font_u8glib_4_tf );
    //u8g2.drawStr(3, 62, "RX BUF.=");
    //u8g2.setCursor(30, 54);
    //u8g2.print(BUF.toInt());

    u8g2.setFont(u8g2_font_profont10_tn);

    if (X_MPOS.toFloat() >= 100) {
      u8g2.setCursor(55, 48);
      u8g2.print(X_MPOS.toFloat(), 1);
    }
    else {
      if (X_MPOS.toFloat() >= 10) {
        u8g2.setCursor(60, 48);
        u8g2.print(X_MPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(65, 48);
        u8g2.print(X_MPOS.toFloat(), 1);
      }
    }



    if (Y_MPOS.toFloat() >= 100) {
      u8g2.setCursor(55, 55);
      u8g2.print(Y_MPOS.toFloat(), 1);
    }
    else
    {
      if (Y_MPOS.toFloat() >= 10) {
        u8g2.setCursor(60, 55);
        u8g2.print(Y_MPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(65, 55);
        u8g2.print(Y_MPOS.toFloat(), 1);
      }
    }



    if (Z_MPOS.toFloat() >= 100) {
      u8g2.setCursor(55, 62);
      u8g2.print(Z_MPOS.toFloat(), 1);
    }
    else
    {
      if (Z_MPOS.toFloat() >= 10) {
        u8g2.setCursor(60, 62);
        u8g2.print(Z_MPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(65, 62);
        u8g2.print(Z_MPOS.toFloat(), 1);
      }
    }


    if (X_WPOS.toFloat() >= 100) {
      u8g2.setCursor(102, 48);
      u8g2.print(X_WPOS.toFloat(), 1);
    }
    else {
      if (X_WPOS.toFloat() >= 10) {
        u8g2.setCursor(107, 48);
        u8g2.print(X_WPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(112, 48);
        u8g2.print(X_WPOS.toFloat(), 1);
      }
    }


    if (Y_WPOS.toFloat() >= 100) {
      u8g2.setCursor(102, 55);
      u8g2.print(Y_WPOS.toFloat(), 1);
    }
    else
    {
      if (Y_WPOS.toFloat() >= 10) {
        u8g2.setCursor(107, 55);
        u8g2.print(Y_WPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(112, 55);
        u8g2.print(Y_WPOS.toFloat(), 1);
      }
    }



    if (Z_WPOS.toFloat() >= 100) {
      u8g2.setCursor(102, 62);
      u8g2.print(Z_WPOS.toFloat(), 1);
    }
    else
    {
      if (Z_WPOS.toFloat() >= 10) {
        u8g2.setCursor(107, 62);
        u8g2.print(Z_WPOS.toFloat(), 1);
      }
      else
      {
        u8g2.setCursor(112, 62);
        u8g2.print(Z_WPOS.toFloat(), 1);
      }
    }

    u8g2.drawRFrame(0, 40, 128, 24, 1); // machine status frame
    u8g2.drawBox(1, 41, 26, 12); // machine status title box
    u8g2.drawTriangle(27, 40, 27, 52, 33, 40); // machine status title triangle

    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_micro_mr);
    u8g2.drawStr(2, 46, "MACHINE");
    u8g2.drawStr(2, 52, "STATUS");
    u8g2.setDrawColor(1);

  } while ( u8g2.nextPage() );

}

void Split(String SplitText, String SplitChar) {
  SplitText = SplitChar + SplitText ;
  int SplitCount = SplittingArraySize;
  int SplitIndex = -1;
  int SplitIndex2;
  //
  for (int i = 0; i < SplitCount - 1; i++) {
    SplitIndex = SplitText.indexOf(SplitChar, SplitIndex + 1);
    SplitIndex2 = SplitText.indexOf(SplitChar, SplitIndex + 1);
    if (SplitIndex2 < 0) SplitIndex2 = SplitText.length() ;
    SplittingArray[i] = SplitText.substring(SplitIndex + 1, SplitIndex2);
  }
}

