// Kareem Adel

#include <Time.h>

time_t elapsedTime;
//setTime(0);

#include "U8glib.h"

U8GLIB_PCD8544 u8g(6, 5, 3, 4, 2);
//U8GLIB_PCD8544 u8g(SCLK, MOSI, CE, D/C, RST)

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 12 on the Arduino
#define ONE_WIRE_BUS 12

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int t; // main temp
int LastRead = 0; // last read temp
int tempmax = 0; // maximum temp recorded
int tempmin = 0; // minimum temp recorded
int tstate = 0; // state of temp, increasing or decreasing. Values 0 for stable, 1 for increasing and -1 for decreasing
int talarm = 35; // temp at which alarm is raised

void setup() {

  sensors.begin();  // Start up the library
  Serial.begin(9600);  // Start up the serial monitor
  pinMode (13, OUTPUT); // alarm buzzer connected to pin 13
  sensors.requestTemperatures();
  sensors.setResolution(9); // set resolution of temp bits (from 9 to 12)
  tempmin = tempmax = sensors.getTempCByIndex(0);
}

void loop() {

  elapsedTime = now();
  sensors.requestTemperatures();
  t = sensors.getTempCByIndex(0);

  if (t >= talarm) {
    // sound the alarm for a pulse
    digitalWrite (9, HIGH);
    delay (50);
    digitalWrite (9, LOW);
    digitalWrite (13, HIGH);
    delay (600);
    digitalWrite (13, LOW);
  }

  else {
    digitalWrite (13, LOW);
  }

  if (LastRead - t > 0) {
    tstate = 1;
  }

  if (LastRead - t < 0) {
    tstate = -1;
  }

  if (LastRead - t == 0) {
    tstate = 0;
  }

  if (t > tempmax) {
    tempmax = LastRead;
  }

  if (t < tempmin) {
    tempmin = LastRead;
  }

  Serial.print("Current temperature = ");
  Serial.print(t);
  Serial.println(" C");

  // picture loop
  u8g.firstPage();
  do {
    u8g.setContrast(128); // contrast of LCD 5110

    u8g.setFont(u8g_font_chikita);
    u8g.drawStr(0, 5, "Current");
    u8g.drawStr(0, 11, "LASER Temp.");
    u8g.drawStr(0, 19, "Maximum");
    u8g.drawStr(0, 25, "LASER Temp.");
    u8g.drawStr(0, 33, "Minimum");
    u8g.drawStr(0, 39, "LASER Temp.");

    u8g.setFont(u8g_font_helvB14);
    u8g.setPrintPos(65, 13);
    u8g.print(float(t));
    u8g.setFont(u8g_font_helvR10);
    u8g.setPrintPos(69, 26);
    u8g.print(float(tempmax));
    u8g.setPrintPos(69, 40);
    u8g.print(float(tempmin));

    u8g.drawLine(0, 41, 84, 41); // draw horizontal line
    u8g.drawLine(42, 41, 42, 48); // draw vertical line

    u8g.setFont(u8g_font_04b_03br);
    u8g.drawStr(12, 48, ":");
    u8g.drawStr(27, 48, ":");
    if (hour(elapsedTime) >= 10) {
      u8g.setPrintPos(0, 48);
      u8g.print(hour(elapsedTime));
    }
    else {
      u8g.drawStr(0, 48, "0");
      u8g.setPrintPos(5, 48);
      u8g.print(hour(elapsedTime));
    }
    if (minute(elapsedTime) >= 10) {
      u8g.setPrintPos(15, 48);
      u8g.print(minute(elapsedTime));
    }
    else {
      u8g.drawStr(15, 48, "0");
      u8g.setPrintPos(20, 48);
      u8g.print(minute(elapsedTime));
    }
    if (second(elapsedTime) >= 10) {
      u8g.setPrintPos(31, 48);
      u8g.print(second(elapsedTime));
    }
    else {
      u8g.drawStr(31, 48, "0");
      u8g.setPrintPos(36, 48);
      u8g.print(second(elapsedTime));
    }

    u8g.setFont(u8g_font_6x13_75r);
    u8g.setPrintPos(58, 11);
    if (tstate == -1) {
      u8g.print("2"); // draw down arrow
    }
    if (tstate == 1) {
      u8g.print("<"); // draw up arrow
    }
    if (t >= talarm)
    {
      u8g.setFont(u8g_font_04b_03r);
      u8g.setPrintPos(75, 48);
      u8g.print(talarm);
      u8g.drawStr(45, 48, "ALARM!");
    }
    else {
      u8g.setFont(u8g_font_04b_03r);
      u8g.setPrintPos(75, 48);
      u8g.drawStr(55, 48, "MAX");
      u8g.print(talarm);
    }

  } while ( u8g.nextPage() );

  LastRead = t;

  delay(150);
}

