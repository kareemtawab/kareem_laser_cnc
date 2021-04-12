
#include <Arduino.h>
#include <U8g2lib.h>
#include "Timer.h"

Timer t;

U8G2_ST7920_128X64_2_SW_SPI u8g2(U8G2_R0, 10, 9, 8, 7);
/*
  Constructor template used: U8G2_ST7920_128X64_2_SW_SPI(rotation, clock, data, cs [, reset])

  E = Clock          pin 11
  RW = Data = MOSI   pin 10
  RS = Chip Select   pin 9
  RST = Reset        pin 8
  PSB                GND for serial interface
*/

void setup() {

  t.every(20, u8g2.getMenuEvent());
  pinMode(6, OUTPUT);
  analogWrite(6, 150);
}
void loop() {

  t.update();
  u8g2.begin(/* menu_select_pin= */ 3, /* menu_next_pin= */ U8X8_PIN_NONE, /* menu_prev_pin= */ U8X8_PIN_NONE, /* menu_up_pin= */ 4, /* menu_down_pin= */ 12, /* menu_home_pin= */ U8X8_PIN_NONE);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.userInterfaceSelectionList("Title", 1, "abcdef\nghijkl\nmnopqr");
  } while ( u8g2.nextPage() );

}
