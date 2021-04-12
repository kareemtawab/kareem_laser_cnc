/*
  ##################################################
  ##  LCD DUAL TEMPERATURE MONITOR, TIME COUNTER  ##
  ##         AND CLOCK FOR LASER MODULE           ##
  ##                                              ##
  ##   by         H     H K   K    888    999     ##
  ##              H     H K  K    8   8  9   9    ##
  ##              H     H KKK      888   9   9    ##
  ##              HHHHHHH K  K    8   8   9999    ##
  ##              H     H K   K   8   8      9    ##
  ##              H     H K    K   888    999     ##
  ##################################################

  This sketch grabs the temperature of the heatsinks of a 6W+ LASER diode NUBM44
  and a LASER driver IC and prints them on a ST7920 LCD. It also counts the time
  the system is turned on and records to EEPROM the accumilative hours counted
  every predefined number of minutes and prints them on LCD.

  LCD hookup guide on https://github.com/olikraus/u8g2/wiki/setup_tutorial
  DS18B20 wiring guide on https://www.tweaking4all.com/hardware/arduino/arduino-ds18b20-temperature-sensor/

  Big thanks and credits to all authors of the libraries I used.
  ___________________________________________________________________________
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Time.h>
#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Timer.h> // the beautiful library by Simon Monk, Damian Philipp and Jack Christensen. Big thanks to them.
#include <Average.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;
DateTime clockTime;

Timer t;

time_t elapsedTime; // elapsed timer structure

U8G2_ST7920_128X64_2_SW_SPI u8g2(U8G2_R0, 10, 9, 8, 7);
/*
  Constructor template used: U8G2_ST7920_128X64_2_SW_SPI(rotation, clock, data, cs [, reset])

  E = Clock          pin 10
  RW = Data = MOSI   pin 9
  RS = Chip Select   pin 8
  RST = Reset        pin 7
  PSB                GND for serial interface
*/


// Reserve space for 20 entries in the average bucket.
// Change the type between < and > to change the entire way the library works.
#define entries 3
Average<float> voltavg(entries);
Average<float> currentavg(entries);

char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

#define address 10 // EEPROM memory address to record on counted hours
#define address2 50 // EEPROM memory address hours counter record interval
#define address3 100 // EEPROM memory address current calibration offset

// Data wire is plugged into pin 12 on the Arduino
#define ONE_WIRE_BUS 11 // pin where DS18B20 sensors are attached to 
#define ambient 0 // 1st temp sensor index
#define heatsink 1 // 2nd temp sensor index
#define BL 6 // LCD Backlight pin
#define BUZZER 13 // Buzzer pin

// Setup a oneWire instance to communicate with any OneWire devices
// Wiring mode: parasite mode
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int hrs; // hours of laser life
int hrsrecorded; // hours of laser life on EEPROM
unsigned int accu = 0;
bool abovethreshold = 0;
long recordminutes = 10; // minutes to record on EEPROM. It has to be a long because the autors recommend that for stability.
int t1; // current temp of NUBM44
int t2; // current temp of SXD
int t1prev = 0; // previous temp of NUBM44
int t2prev = 0; // previous temp of SXD
float currentcaloffset;
float v = 0.0f;
float i = 0.0f;
float vavg = 0.0f;
float iavg = 0.0f;
int usage;
int e;
float tpower = 0.000f;
float opower = 0.000f;

static const unsigned char intro[] PROGMEM =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
  0x00, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x00, //                               ###
  0x00, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x00, //                               ####
  0x00, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x00, //                              ######
  0x00, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x00, //                              ######
  0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0x00, //                             ########
  0x00, 0x00, 0x00, 0x78, 0x0F, 0x00, 0x00, 0x00, //                            #### ####
  0x00, 0x00, 0x00, 0x78, 0x1E, 0x00, 0x00, 0x00, //                            ####  ####
  0x00, 0x00, 0x00, 0x3C, 0x1E, 0x00, 0x00, 0x00, //                           ####   ####
  0x00, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x00, //                           ####    ####
  0x00, 0x00, 0x00, 0x1E, 0x78, 0x00, 0x00, 0x00, //                          ####      ####
  0x00, 0x00, 0x00, 0x1E, 0x78, 0x00, 0x00, 0x00, //                          ####      ####
  0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, //                         ####        ####
  0x00, 0x00, 0x80, 0x07, 0xF0, 0x00, 0x00, 0x00, //                        ####         ####
  0x00, 0x00, 0x80, 0x07, 0xE0, 0x01, 0x00, 0x00, //                        ####          ####
  0x00, 0x00, 0xC0, 0x03, 0xC0, 0x03, 0x00, 0x00, //                       ####            ####
  0x00, 0x00, 0xC0, 0x03, 0xC0, 0x03, 0x00, 0x00, //                       ####            ####
  0x00, 0x00, 0xE0, 0x01, 0x80, 0x07, 0x00, 0x00, //                      ####              ####
  0x00, 0x00, 0xE0, 0x01, 0x80, 0x07, 0x00, 0x00, //                      ####              ####
  0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, //                     ####                ####
  0x00, 0x00, 0x78, 0x00, 0x00, 0x0F, 0x00, 0x00, //                    ####                 ####
  0x00, 0x00, 0x78, 0x80, 0x00, 0x1E, 0x00, 0x00, //                    ####        #         ####
  0x00, 0x00, 0x3C, 0x80, 0x01, 0x3C, 0x00, 0x00, //                   ####         ##         ####
  0x00, 0x00, 0x3C, 0x82, 0x20, 0x3C, 0x00, 0x00, //                   ####   #     #     #    ####
  0x00, 0x00, 0x1E, 0x86, 0x60, 0x78, 0x00, 0x00, //                  ####    ##    #     ##    ####
  0x00, 0x00, 0x0F, 0x86, 0x30, 0x78, 0x00, 0x00, //                 ####     ##    #    ##     ####
  0x00, 0x00, 0x0F, 0xAC, 0x12, 0xF0, 0x00, 0x00, //                 ####      ## # # #  #       ####
  0x00, 0x80, 0x07, 0xA8, 0x1A, 0xF0, 0x00, 0x00, //                ####        # # # # ##       ####
  0x00, 0x80, 0x37, 0xFA, 0x2A, 0xE6, 0x01, 0x00, //                #### ##   # ##### # # #   ##  ####
  0x00, 0xC0, 0xE3, 0xD6, 0x36, 0xC7, 0x03, 0x00, //               ####   ### ## # ## ## ##  ###   ####
  0x00, 0xC0, 0xC3, 0xED, 0xDF, 0xC1, 0x03, 0x00, //               ####    ### ## ######## ###     ####
  0x00, 0xE0, 0x01, 0xF7, 0x6F, 0x80, 0x07, 0x00, //              ####       ### ######## ##        ####
  0x00, 0xF0, 0x80, 0xFD, 0x9F, 0x80, 0x07, 0x00, //             ####       ## ###########  #       ####
  0x00, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x0F, 0x00, //             ####        ################        ####
  0x00, 0x78, 0xF8, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, //            ####    #################################
  0x00, 0x78, 0xF8, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, //            ####    ##################################
  0x00, 0x3C, 0x00, 0xFE, 0x7F, 0x00, 0x3C, 0x00, //           ####           ##############           ####
  0x00, 0x3C, 0x80, 0xF9, 0xDF, 0x01, 0x3C, 0x00, //           ####         ##  ########## ###         ####
  0x00, 0x1E, 0x00, 0xF6, 0x37, 0x00, 0x78, 0x00, //          ####            ## ####### ##             ####
  0x00, 0x0F, 0x80, 0xEB, 0xCF, 0x00, 0x78, 0x00, //         ####           ### # #######  ##           ####
  0x00, 0x0F, 0xE0, 0xF4, 0xB6, 0x03, 0xF0, 0x00, //         ####         ###  # #### ## ## ###          ####
  0x80, 0x07, 0x30, 0xF2, 0x2A, 0x06, 0xE0, 0x01, //        ####         ##   #  #### # # #   ##          ####
  0x80, 0x07, 0x00, 0xA8, 0x0A, 0x00, 0xE0, 0x01, //        ####                # # # # #                 ####
  0xC0, 0x03, 0x00, 0xAC, 0x16, 0x00, 0xC0, 0x03, //       ####                ## # # ## #                 ####
  0xC0, 0x03, 0x00, 0x84, 0x30, 0x00, 0xC0, 0x03, //       ####                #    #    ##                ####
  0xE0, 0x01, 0x00, 0x86, 0x20, 0x00, 0x80, 0x07, //      ####                ##    #     #                 ####
  0xF0, 0x00, 0x00, 0x82, 0x60, 0x00, 0x80, 0x07, //     ####                 #     #     ##                ####
  0xF0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x0F, //     ####                       #                        ####
  0x78, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x1E, //    ####                        #                         ####
  0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, //    ####                                                  ####
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, //   ############################################################
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, //   ############################################################
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, //   ############################################################
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
  0x08, 0xBB, 0x1B, 0xC6, 0xE6, 0xEC, 0x4E, 0x06, //    #    ## ### ### ##    ##   ## ##  ###  ## ### ###  #  ##
  0x88, 0x8A, 0x28, 0xAA, 0x4A, 0x4A, 0xA4, 0x0A, //    #   # # #   #   # #   # # # # # #  #  # #  #   #  # # # #
  0x88, 0x8A, 0x28, 0xAA, 0x4A, 0x4A, 0xA4, 0x0A, //    #   # # #   #   # #   # # # # # #  #  # #  #   #  # # # #
  0x88, 0xBB, 0x19, 0xE6, 0x4A, 0x4E, 0xA4, 0x0A, //    #   ### ### ##  ##    ##  ### # #  #  ###  #   #  # # # #
  0x88, 0xA2, 0x28, 0xAA, 0x4A, 0x4A, 0xA4, 0x0A, //    #   # #   # #   # #   # # # # # #  #  # #  #   #  # # # #
  0x88, 0xA2, 0x28, 0xAA, 0x4A, 0x4A, 0xA4, 0x0A, //    #   # #   # #   # #   # # # # # #  #  # #  #   #  # # # #
  0xB8, 0xBA, 0x2B, 0xAA, 0xE6, 0x4A, 0x4E, 0x0A, //    ### # # ### ### # #   # # # # ##  ### # #  #  ###  #  # #
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
};

void setup() {

  Wire.begin();
  rtc.begin();
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  sensors.begin();  // Start up the library
  sensors.setResolution(9); // set resolution of temp bits (from 9 to 12)
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  Serial.begin(115200);

  Serial.println(F("LCD DUAL TEMPERATURE MONITOR, WATT METER, TIME COUNTER AND CLOCK FOR LASER MODULE BY HK89"));
  Serial.print(F("Compile date: "));
  Serial.print(__DATE__);
  Serial.print(F(", "));
  Serial.println(__TIME__);
  clockTime = rtc.now();
  Serial.print(F("RTC time is: "));
  Serial.print(daysOfTheWeek[clockTime.dayOfTheWeek()]);
  Serial.print(" ");
  Serial.print(clockTime.day(), DEC);
  Serial.print('/');
  Serial.print(clockTime.month(), DEC);
  Serial.print('/');
  Serial.print(clockTime.year(), DEC);
  Serial.print(", ");
  Serial.print(clockTime.hour(), DEC);
  Serial.print(':');
  Serial.print(clockTime.minute(), DEC);
  Serial.print(':');
  Serial.println(clockTime.second(), DEC);
  Serial.println();

  EEPROM.get(address, hrsrecorded);
  if (isnan(hrsrecorded)) {
    hrsrecorded = 0;
    EEPROM.put(address, hrsrecorded);
    Serial.println(F("Hours counter reset!"));
    Serial.println(F(""));
  }

  for (byte i = 0; i < sizeof(currentcaloffset); i++) {
    reinterpret_cast<byte*>(&currentcaloffset)[i] = EEPROM.read(address3 + i);
  }
  if (isnan(currentcaloffset)) {
    currentcaloffset = 0;
    for (byte i = 0; i < sizeof(currentcaloffset); i++) {
      EEPROM.write(address3 + i, reinterpret_cast<byte*>(&currentcaloffset)[i]);
    }
    Serial.print(F("Current calibration offset set as "));
    Serial.print(currentcaloffset);
    Serial.println(F(" amps"));
  }
  Serial.println(F("To set the RTC time, type in the letter 'T'"));
  Serial.println(F("To manually set the recorded hours, type in the letter 'H'"));
  Serial.println(F("To set the current calibration offset, type in the letter 'C'"));

  u8g2.begin(
    /* menu_select_pin= */ U8X8_PIN_NONE,
    /* menu_next_pin= */ U8X8_PIN_NONE,
    /* menu_prev_pin= */ U8X8_PIN_NONE,
    /* menu_up_pin= */ U8X8_PIN_NONE,
    /* menu_down_pin= */ U8X8_PIN_NONE,
    /* menu_home_pin= */ U8X8_PIN_NONE);
  drawintro();

  // This set of functions allows the user to change the date and time
  if (Serial.peek() == 't' || Serial.peek() == 'T') {
    serialclearbuffer();
    setTime();
    delay(1000);
    Serial.print(F("Settings saved! RTC time now is: "));
    Serial.print(daysOfTheWeek[clockTime.dayOfTheWeek()]);
    Serial.print(" ");
    Serial.print(clockTime.day(), DEC);
    Serial.print('/');
    Serial.print(clockTime.month(), DEC);
    Serial.print('/');
    Serial.print(clockTime.year(), DEC);
    Serial.print(", ");
    Serial.print(clockTime.hour(), DEC);
    Serial.print(':');
    Serial.print(clockTime.minute(), DEC);
    Serial.print(':');
    Serial.println(clockTime.second(), DEC);
    Serial.println();
  }

  else if (Serial.peek() == 'h' || Serial.peek() == 'H') {
    serialclearbuffer();
    sethours();
    delay(1000);
    Serial.println(F("Settings saved!"));
  }

  else if (Serial.peek() == 'c' || Serial.peek() == 'C') {
    serialclearbuffer();
    setcurrentcaloffset();
    delay(1000);
    Serial.println(F("Settings saved!"));
  }

  Serial.println(F(""));
  Serial.println(F("System starting:"));
  digitalWrite(BUZZER, HIGH);
  delay(2000);
  digitalWrite(BUZZER, LOW);

  Serial.print(F("System will record LASER hours to EEPROM in hours every "));
  Serial.print(recordminutes);
  Serial.println(F(" min(s)"));
  Serial.print(F("Previous LASER hours recorded on EEPROM = "));
  Serial.println(hrsrecorded);
  Serial.print(F("Current calibration offset = "));
  Serial.println(currentcaloffset, 1);

  t.every(recordminutes * 60 * 1000, reocordtoEEPROM);
  t.every(500, dallastempget);
  t.every(150, calcwatts);
  t.every(1000, elapsedsecs);

  currentcaloffset = -.37;
  analogWrite(BL, 150);
}

void loop() {

  clockTime = rtc.now();
  hrs = hrsrecorded + (accu / 3600);
  t.update();

  u8g2.firstPage();
  do {
    //____________________________________
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 10);
    u8g2.setDrawColor(0);
    //____________________________________
    byte dateposx = 1;
    byte dateposy = 8;
    u8g2.setFont(u8g2_font_profont10_mr);
    u8g2.setCursor(dateposx, dateposy);
    u8g2.drawStr(dateposx, dateposy, daysOfTheWeek[clockTime.dayOfTheWeek()]);
    u8g2.drawStr(dateposx + 25, dateposy + 0, ".");
    if (clockTime.day() >= 10) {
      u8g2.setCursor(dateposx + 17, dateposy + 0);
      u8g2.print(clockTime.day());
    }
    else {
      u8g2.drawStr(dateposx + 17, dateposy + 0, "0");
      u8g2.setCursor(dateposx + 22, dateposy + 0);
      u8g2.print(clockTime.day());
    }
    if (clockTime.month() >= 10) {
      u8g2.setCursor(dateposx + 29, dateposy + 0);
      u8g2.print(clockTime.month());
    }
    else {
      u8g2.drawStr(dateposx + 29, dateposy + 0, "0");
      u8g2.setCursor(dateposx + 34, dateposy + 0);
      u8g2.print(clockTime.month());
    }
    //____________________________________
    byte runposx = 48;
    byte runposy = 8;
    u8g2.drawStr(runposx + 9, runposy + 0, ":");
    u8g2.drawStr(runposx + 23, runposy + 0, ":");
    if (hour(elapsedTime) >= 10) {
      u8g2.setCursor(runposx + 0, runposy + 0);
      u8g2.print(hour(elapsedTime));
    }
    else {
      u8g2.drawStr(runposx + 0, runposy + 0, "0");
      u8g2.setCursor(runposx + 5, runposy + 0);
      u8g2.print(hour(elapsedTime));

    }
    if (minute(elapsedTime) >= 10) {
      u8g2.setCursor(runposx + 14, runposy + 0);
      u8g2.print(minute(elapsedTime));
    }
    else {
      u8g2.drawStr(runposx + 14, runposy + 0, "0");
      u8g2.setCursor(runposx + 19, runposy + 0);
      u8g2.print(minute(elapsedTime));
    }
    if (second(elapsedTime) >= 10) {
      u8g2.setCursor(runposx + 28, runposy + 0);
      u8g2.print(second(elapsedTime));
    }
    else {
      u8g2.drawStr(runposx + 28, runposy + 0, "0");
      u8g2.setCursor(runposx + 33, runposy + 0);
      u8g2.print(second(elapsedTime));
    }
    //____________________________________
    byte timeposx = 105;
    byte timeposy = 8;
    u8g2.drawStr(timeposx + 8, timeposy + 0, ":");
    if (clockTime.hour() >= 10) {
      u8g2.setCursor(timeposx + 0, timeposy + 0);
      u8g2.print(clockTime.hour());
    }
    else {
      u8g2.drawStr(timeposx + 0, timeposy + 0, "0");
      u8g2.setCursor(timeposx + 5, timeposy + 0);
      u8g2.print(clockTime.hour());
    }
    if (clockTime.minute() >= 10) {
      u8g2.setCursor(timeposx + 12, timeposy + 0);
      u8g2.print(clockTime.minute());
    }
    else {
      u8g2.drawStr(timeposx + 12, timeposy + 0, "0");
      u8g2.setCursor(timeposx + 17, timeposy + 0);
      u8g2.print(clockTime.minute());
    }
    //____________________________________
    u8g2.setDrawColor(1);
    //____________________________________
    u8g2.setFont(u8g2_font_baby_tf  );
    u8g2.drawStr(0, 18, "Airflow Temp  (degC)");
    u8g2.drawStr(0, 25, "Heatsink Temp  (degC)");
    u8g2.drawStr(0, 36, "Actual Power  (mW)");
    u8g2.drawStr(0, 43, "Calculated Power  (mW)");
    u8g2.drawStr(0, 50, "Usage  (%)");
    u8g2.drawStr(0, 57, "Efficiency  (%)");
    u8g2.drawStr(0, 64, "Service Hours");
    u8g2.drawHLine(2, 28, 124);
    u8g2.setCursor(100, 18);
    u8g2.print(t1, 1);
    u8g2.setCursor(100, 25);
    u8g2.print(t2, 1);
    u8g2.setCursor(100, 36);
    u8g2.print(tpower, 0);
    u8g2.setCursor(100, 43);
    if (abovethreshold) {
      u8g2.drawStr(95, 51, "!");
    }
    u8g2.print(opower, 0);
    u8g2.setCursor(100, 50);
    u8g2.print(usage);
    u8g2.setCursor(100, 57);
    u8g2.print(e);
    u8g2.setCursor(100, 64);
    u8g2.print(hrs);

  } while ( u8g2.nextPage() );

  t1prev = t1;
  t2prev = t2;

}

void reocordtoEEPROM() {

  EEPROM.put(address, hrs);
  Serial.print(F("Recorded LASER life to EEPROM at "));
  Serial.print(hour(elapsedTime));
  Serial.print(F(" : "));
  Serial.print(minute(elapsedTime));
  Serial.print(F(" : "));
  Serial.print(second(elapsedTime));
  Serial.print(F(" run time ("));
  Serial.print(hrs, 3);
  Serial.println(F(" hours)"));

}

void dallastempget() {

  t1 = sensors.getTempCByIndex(heatsink);
  t2 = sensors.getTempCByIndex(ambient);
  if (t1 == -127) {
    t1 = t1prev;
  }
  if (t2 == -127) {
    t2 = t1prev;
  }
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

}

void calcwatts() {

  unsigned int ADCforV;
  unsigned int ADCforI;
  double Vcc;
  Vcc = readVcc() / 1000.0;

  ADCforV = analogRead(A0);
  v = ADCforV * Vcc / 1024;
  v = v * 5; // convertion ratio for voltage sense board = 5 / 25
  ADCforI = analogRead(A1);
  i = ADCforI * Vcc / 1024;
  voltavg.push(v);
  currentavg.push(i);
  vavg = voltavg.mean();
  iavg = currentavg.mean();
  //Serial.print(F("V = "));
  //Serial.print(vavg);
  //Serial.print(F(" A = "));
  //Serial.println(iavg);
  tpower = (iavg + currentcaloffset) * vavg * 1000;
  if (tpower < 0) {
    tpower = 0;
  }
  opower = -0.0000000002 * pow(tpower, 3) - 0.000003 * pow(tpower, 2) + 0.4814 * tpower; // equation derived from dtr laser profile excel sheet
  usage = opower / 6000 * 100;
  if (usage > 150) {
    usage = 150;
  }
  e = (opower / tpower) * 100;
  if (e == NAN) {
    e = 0;
  }

}

void elapsedsecs() {
  elapsedTime++;
  if (usage >= 10) {
    accu++;
    abovethreshold = true;
  }
  else {
    abovethreshold = false;
  }
}


void setTime() {

  Serial.print(F("Set year, (00)-(38). - "));
  int y = readSerial();
  Serial.println(y);
  Serial.print(F("Set month, (01)-(12). - "));
  int mo = readSerial();
  Serial.println(mo);
  Serial.print(F("Set day, (01)-(31). - "));
  int d = readSerial();
  Serial.println(d);
  Serial.print(F("Set hour in 24hr format, (00)-(23). - "));
  int h = readSerial();
  Serial.println(h);
  Serial.print(F("Set minute, (00)-(59). - "));
  int m = readSerial();
  Serial.println(m);
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  rtc.adjust(DateTime(y, mo, d, h, m, 0));
  clockTime = rtc.now();

}

void  sethours() {
  Serial.print(F("Set hours, (0)-(9999). - "));
  serialclearbuffer();
  hrsrecorded = readSerial();
  if (hrsrecorded >= 0.00 && hrsrecorded <= 9999.00) {
    EEPROM.put(address2, hrsrecorded);
    Serial.println(hrsrecorded);
  }
  else {
    Serial.print(hrsrecorded);
    Serial.println(F(" Out of range. Set as zero"));
    EEPROM.put(address2, 0);
  }
}

void  setcurrentcaloffset() {
  Serial.print(F("Set current calibration offset, (-2.0)-(2.0). - "));
  serialclearbuffer();
  currentcaloffset = readSerial();
  if (currentcaloffset >= -2.0 && currentcaloffset <= 2.0) {
    for (byte i = 0; i < sizeof(currentcaloffset); i++) {
      EEPROM.write(address3 + i, reinterpret_cast<byte*>(&currentcaloffset)[i]);
    }
    Serial.println(currentcaloffset);
  }
  else {
    Serial.print(currentcaloffset);
    Serial.println(F(" Out of range. Set as zero"));
    currentcaloffset = 0;
    for (byte i = 0; i < sizeof(currentcaloffset); i++) {
      EEPROM.write(address3 + i, reinterpret_cast<byte*>(&currentcaloffset)[i]);
    }
  }
}

byte readSerial() {

  while (!Serial.available()) delay(10);
  float reading = Serial.parseFloat();
  serialclearbuffer();
  return reading;

}

void serialclearbuffer() {

  for (int x = 0 ; x < 150 ; x++) {
    Serial.read();
  }

}

void drawintro() {

  analogWrite(BL, 150);

  for (int x = 127; x > 50; x--) {
    delay(5);
    u8g2.firstPage();
    do {
      u8g2.drawBox(x, 34, 127 - x, 2);
      //u8g2.drawXBMP( 0, 0, 128, 64, intro);
    } while ( u8g2.nextPage() );
  }
  delay(100);
  analogWrite(BL, 50);
  delay(50);
  analogWrite(BL, 150);
  delay(50);

  u8g2.firstPage();
  do {
    u8g2.drawXBMP( 0, 0, 64, 64, intro);
    u8g2.drawBox(50, 34, 127 - 50, 2);
    u8g2.setFont(u8g2_font_saikyosansbold8_8u);
    u8g2.drawStr(50, 8, "LASERGRBL");
  } while ( u8g2.nextPage() );

  delay(1000);

  u8g2.firstPage();
  do {
    u8g2.drawXBMP( 0, 0, 64, 64, intro);
    u8g2.drawBox(50, 34, 127 - 50, 2);
    u8g2.drawStr(50, 8, "LASERGRBL");
    u8g2.setFont(u8g2_font_profont10_mr);
    u8g2.drawStr(45, 16, "TEMP. MONITORING");
    u8g2.drawStr(54, 24, "TIME COUNTING");
    u8g2.drawStr(65, 32, "AND CLOCK");
    u8g2.drawStr(108, 63, "HK89");
  } while ( u8g2.nextPage() );

}

long readVcc() {

  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(5); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;

}
