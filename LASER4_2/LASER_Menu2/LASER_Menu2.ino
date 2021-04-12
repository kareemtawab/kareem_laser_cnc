/*

  Debounce.ino

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <Arduino.h>
//#include <SPI.h>
#include <U8g2lib.h>

U8G2_ST7920_128X64_2_SW_SPI u8g2(U8G2_R0, 4, 5, 6, 7);

void setup(void) {
  pinMode(6, OUTPUT);
  analogWrite(6, 150);
  u8g2.begin(/* menu_select_pin= */ 8, /* menu_next_pin= */ 9, /* menu_prev_pin= */ 10, /* menu_up_pin= */ 12, /* menu_down_pin= */ 13, /* menu_home_pin= */ U8X8_PIN_NONE);
  u8g2.setFont(u8g2_font_helvB12_tr);
  Serial.begin(115200);
}

uint8_t last_event = 0;
uint16_t cnt = 0;

void draw()
{
  u8g2.firstPage();
  do {
    u8g2.setCursor(0, 24);
    u8g2.print((int)last_event);

    u8g2.setCursor(0, 48);
    u8g2.print(cnt);
  } while ( u8g2.nextPage() );
  cnt++;
}


void loop(void) {
  uint8_t e;
  e = u8g2.getMenuEvent();
  if ( e != 0 )
  {
    last_event = e;
    Serial.println((int)last_event);
  }
  //draw();
}

