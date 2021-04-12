/*
   MARK VAN VEGGEL
   ARDUINO LCD MENU V0.1
   YOUTUBE.COM/AMPSOURCE

*/

#include <OneButton.h>
#include <Encoder.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C  lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

// ROTARY ENCODER
long oldPosition  = 0;
int encoderDTpin = 2;
int encoderCLKpin = 3;
Encoder myEnc(encoderDTpin, encoderCLKpin);

int buttonPin = 5;
OneButton button0(buttonPin, true);

#define emergencybutton 13
#define item1 11 // unused item 1
#define item2 10 // unused item 2
#define item3 9 // laser
#define item4 8 // lights
#define item5 7 // system fans
#define item6 6 // motors

// MENU ARRAY
// First number in array = amount of menu items in root layer. Same goes for next layer. (Example: first value in array is 4. This means Layer 0 has 4 menu items. Layer 1 has only 2 menu items.)
// First number infront of menu text is the layer number that item points to. (Example: 1_Rotation points to Layer 1)
// Second number infront of menu text is the number of the value connected to that menu item in the values array. (Example: 15Direction points to position 5 in the values array)

String menu[] = {"6", "00_Motors", "04_System Fans", "08_Lights", "012LASER", "01612V/24V Aux.", "020Unused item", "", ""}; // Layer 0
int itemsstate[7];

// VALUES ARRAY
// TYPES, 1 numeric / 2 character value range
// 1 - type,value,increments,min,max
// 2 - type,starting value in options array,lowest value options array,max value options array
int values[] = {2, 0, 0, 1,      //0-3 Motors
                2, 0, 0, 1,      //4-7 System Fans
                2, 0, 0, 1,      //8-11 Lights
                2, 0, 0, 1,      //12-15 LASER
                2, 0, 0, 1,      //16-19 12V/24V Aux.
                2, 0, 0, 1       //20-23 Unused item
               };


String options[] = {"OFF", "ON"};

// Custom character for LCD.
byte cursor[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11100,
  0b11000,
  0b10000,
  0b00000
};

int currentType = 0;
int value = 0;
int numIncr = 0;
int Min = 0;
int Max = 0;

int currentLayer = 0;
int lastLayer = 99;
int currentLength = menu[0].toInt();
int currentPosition = 0;
int currentSelect = 1;
int currentChange = 0;
String currentPress = "";
float currentlcdLayerFloat = 0;
int currentlcdLayer = 0;
int lastlcdLayer = 0;
int lcdUpdated = 0;
int currentLine = 0;
int cursorLayer = 0;
int cursorPosition = 0;
int valueLength = 0;

void setup() {

  pinMode(13, OUTPUT);
  pinMode(item1, OUTPUT);
  pinMode(item2, OUTPUT);
  pinMode(item3, OUTPUT);
  pinMode(item4, OUTPUT);
  pinMode(item5, OUTPUT);
  pinMode(item6, OUTPUT);
  pinMode(emergencybutton, INPUT);
  digitalWrite(item1, HIGH);
  digitalWrite(item2, HIGH);
  digitalWrite(item3, HIGH);
  digitalWrite(item4, HIGH);
  digitalWrite(item5, LOW);
  digitalWrite(item6, LOW);

  Serial.begin(115200);

  // Map rotary button to actions single and doubleclick.
  button0.attachClick(singleClick);
  button0.attachDoubleClick(doubleClick);

  lcd.begin (20, 4);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home();

  // Create the custom character.
  lcd.createChar(0, cursor);

  lcd.setCursor (2, 0);
  lcd.print("LASER CNC Machine");
  lcd.setCursor (3, 1);
  lcd.print("System Control");
  lcd.setCursor (7, 2);
  lcd.print("Panel");
  lcd.setCursor (0, 3);
  lcd.print("HK89");
  lcd.setCursor (12, 3);
  lcd.print("18.12.17");
  delay(3000);
  digitalWrite(item5, HIGH);
  delay(3000);
  clearLine(0);
  clearSelect();

  // Set the select cursor on the first line.
  cursorLayer = 0;
  lcd.setCursor (0, cursorLayer);
  writeCursor();
  //timereventid = t.oscillate(item5, item5interval, LOW);
}

void loop() {

  if (digitalRead(emergencybutton) == true) {
    digitalWrite(item3, HIGH);
    digitalWrite(item5, LOW);
    digitalWrite(item4, HIGH);
    digitalWrite(item6, LOW);
    digitalWrite(item2, HIGH);
    digitalWrite(item1, HIGH);
    lcd.clear();
    lcd.setCursor (1, 0);
    lcd.print("EMERGENCY SHUTDOWN");
    lcd.setCursor (3, 1);
    lcd.print("BUTTON PRESSED");
    lcd.setCursor (0, 3);
    lcd.print("Reset to continue...");
    while (1);
  }
  // Listen to button presses.
  button0.tick();

  // Listen if the rotary encoder moves.
  rotary_check();

  // Print the LCD menu.
  lcdMenu();

  for (int i = 0; i <= menu[0].toInt() - 1; i ++) {
    itemsstate[i] = values[i + 1 + ((i) * 3)];
  }

  if ( itemsstate[0] == 0) {
    digitalWrite(item3, HIGH);
  }
  if ( itemsstate[0] == 1) {
    digitalWrite(item3, LOW);
  }

  if ( itemsstate[1] == 0) {
    digitalWrite(item5, LOW);
  }
  if ( itemsstate[1] == 1) {
    digitalWrite(item5, HIGH);
  }

  if ( itemsstate[2] == 0) {
    digitalWrite(item4, HIGH);
  }
  if ( itemsstate[2] == 1) {
    digitalWrite(item4, LOW);
  }

  if ( itemsstate[3] == 0) {
    digitalWrite(item6, LOW);
  }
  if ( itemsstate[3] == 1) {
    digitalWrite(item6, HIGH);
  }

  if ( itemsstate[4] == 0) {
    digitalWrite(item2, HIGH);
  }
  if ( itemsstate[4] == 1) {
    digitalWrite(item2, LOW);
  }

  if ( itemsstate[5] == 0) {
    digitalWrite(item1, HIGH);
  }
  if ( itemsstate[5] == 1) {
    digitalWrite(item1, LOW);
  }

  //delay(10);
}

void singleClick() {

  // IF current layer number is the same as redirect layer number, we stay on the same layer
  // AND no item is selected in the menu
  // AND the selected item has a redirect to another layer
  if (currentLayer == menu[currentPosition + currentSelect].substring(0, 1).toInt() && menu[currentPosition + currentSelect].substring(1, 3) != "__" && currentPress == "") {

    currentPress = menu[currentPosition + currentSelect].substring(3);
    currentChange = menu[currentPosition + currentSelect].substring(1, 3).toInt();

    Serial.println("Currentpress: " + currentPress);

    currentType = values[currentChange];
    value = values[currentChange + 1];

    // Type select change is numeric
    if (currentType == 1) {
      Min = values[currentChange + 3];
      Max = values[currentChange + 4];
      numIncr = values[currentChange + 2];

      Serial.println("Currentvalue: " + String(value));
      Serial.println("Increments: " + String(numIncr));
      Serial.println("Min: " + String(Min));
      Serial.println("Max: " + String(Max));


      valueLength = String(value).length();

      // Type select change is character range
    } else if (currentType == 2) {
      Min = values[currentChange + 2];
      Max = values[currentChange + 3];

      Serial.println("Currentvalue: " + options[value]);
      Serial.println("Options: ");

      for (int i = Min; i <= Max; i++) {
        Serial.println(options[i]);
      }

      valueLength = options[value].length();

    }

    clearSelect();
    cursorPosition = 19 - valueLength;
    lcd.setCursor(cursorPosition, cursorLayer);
    writeCursor();

    // ELSE IF something is selected in the menu, and the button is pressed again. We deselect the item.
  } else if (currentPress != "") {

    currentValues();
    currentPress = "";
    currentChange = 0;

    clearChar(cursorPosition, cursorLayer);
    lcd.setCursor(0, cursorLayer);
    writeCursor();

    // IF nothing is selected and nothing will be deselected, move to the redirect layer listed in the first position of the array.
  } else {

    currentLayer = menu[currentPosition + currentSelect].substring(0, 1).toInt();
    currentSelect = 1;
    currentlcdLayerFloat = 0;
    currentlcdLayer = 0;
    lastlcdLayer = 0;

    clearSelect();
    cursorLayer = 0;
    lcd.setCursor (0, cursorLayer);
    writeCursor();

  }

}

// Reset to layer 0.
void doubleClick() {
  if (currentLayer == 0) {
    currentLayer = 0;
    currentSelect = 1;
    currentlcdLayerFloat = 0;
    currentlcdLayer = 0;
    lastlcdLayer = 0;

    clearSelect();
    cursorLayer = 0;

    lcd.clear();
    lcdUpdated = 0;
  }
  if (currentLayer != 0) {
    currentLayer = 0;
    currentSelect = 1;
    currentlcdLayerFloat = 0;
    currentlcdLayer = 0;
    lastlcdLayer = 0;

    clearSelect();
    cursorLayer = 0;
    lcd.setCursor (0, cursorLayer);
    writeCursor();
  }
}

void rotary_check() {

  // Constantly read the position of the rotary encoder
  long newPosition = -1 * myEnc.read() / 4;

  // IF the new position of the encoder is different then the old position
  if (newPosition != oldPosition) {

    // IF nothing has been selected in the menu
    if (currentPress == "") {

      if (newPosition > oldPosition && currentSelect < currentLength) {
        clearSelect();
        currentSelect++;
        currentlcdLayerFloat = currentlcdLayerFloat + 0.25;
        currentlcdLayer = currentlcdLayerFloat;

        cursorSelect();

        lcd.setCursor (0, cursorLayer);
        writeCursor();
        Serial.println("> " + menu[currentPosition + currentSelect].substring(3));

      } else if (newPosition < oldPosition && currentSelect != 1) {
        clearSelect();
        currentSelect--;
        currentlcdLayerFloat = currentlcdLayerFloat - 0.25;
        currentlcdLayer = currentlcdLayerFloat;

        cursorSelect();

        lcd.setCursor (0, cursorLayer);
        writeCursor();
        Serial.println("> " + menu[currentPosition + currentSelect].substring(3));

      }

      if (lastlcdLayer != currentlcdLayer) {
        lcdUpdated = 0;
      }

      lastlcdLayer = currentlcdLayer;
      oldPosition = newPosition;



      // IF something has been selected in the menu, we are going to change the value of a menu item.
    } else if (currentPress != "") {


      lcd.setCursor(cursorPosition + 1, cursorLayer);
      // IF the new position is HIGHER then the old position
      if (newPosition > oldPosition) {
        clearChar(cursorPosition, cursorLayer);

        // IF the selected item has a change type that is NUMERIC.
        // AND the MAXIMUM of the selected item has not been reached.
        // We change the value UPWARDS with the icrements listed in the array.
        // SAME applies for next ELSE IF, just for the change type character. And the next ELSE IF we just go down.
        if (currentType == 1 && value < Max) {
          value = value + numIncr;
          values[currentChange + 1] = value;
          Serial.println(value);

          clearValue();
          valueLength = String(value).length();
          cursorPosition = 19 - valueLength;
          lcd.setCursor(cursorPosition + 1, cursorLayer);
          lcd.print(value);
        }

        if (currentType == 2 && value < Max) {
          value++;
          values[currentChange + 1] = value;
          Serial.println(options[value]);

          clearValue();
          valueLength = options[value].length();
          cursorPosition = 19 - valueLength;
          lcd.setCursor(cursorPosition + 1, cursorLayer);
          lcd.print(options[value]);
        }
        lcd.setCursor(cursorPosition, cursorLayer);
        writeCursor();

      } else {
        clearChar(cursorPosition, cursorLayer);

        if (currentType == 1 && value > Min) {
          value = value - numIncr;
          values[currentChange + 1] = value;
          Serial.println(value);

          clearValue();
          valueLength = String(value).length();
          cursorPosition = 19 - valueLength;
          lcd.setCursor(cursorPosition + 1, cursorLayer);
          lcd.print(value);
        }

        if (currentType == 2 && value > Min) {
          value--;
          values[currentChange + 1] = value;
          Serial.println(options[value]);

          clearValue();
          valueLength = options[value].length();
          cursorPosition = 19 - valueLength;
          lcd.setCursor(cursorPosition + 1, cursorLayer);
          lcd.print(options[value]);
        }
        lcd.setCursor(cursorPosition, cursorLayer);
        writeCursor();

      }



      oldPosition = newPosition;
    } else {
      oldPosition = newPosition;
    }



  }
}

void lcdMenu() {
  if (lastLayer != currentLayer || lcdUpdated == 0) {

    currentLength = menu[0].toInt();
    currentPosition = 0;

    for (int i = 0; i < currentLayer; i++) {
      currentPosition = (menu[currentPosition].toInt() + 1) + currentPosition;
      currentLength = menu[currentPosition].toInt();
    }

    clearMenu();
    int tempPosition = currentPosition;

    if (currentlcdLayer != 0) {

      currentPosition = currentPosition + (currentlcdLayer * 4);
    }

    Serial.println("LCD-------------");

    for (int i = 1; i <= 4; i++) {
      lcd.setCursor (1, i - 1);
      lcd.print(menu[currentPosition + i].substring(3));

      currentType = 0;
      value = values[menu[currentPosition + i].substring(1, 3).toInt() + 1];
      String tempvalue = "";

      if (menu[currentPosition + i].substring(1, 3) != "__" && menu[currentPosition + i].substring(1, 3) != "") {
        currentType = values[menu[currentPosition + i].substring(1, 3).toInt()];
        if (currentType == 1) {
          lcd.setCursor (20 - String(value).length(), i - 1);
          lcd.print(value);
          tempvalue = " " + String(value);
        } else if (currentType == 2) {
          lcd.setCursor (20 - options[value].length(), i - 1);
          lcd.print(options[value]);
          tempvalue = " " + options[value];
        }

      }

      Serial.println(menu[currentPosition + i].substring(3) + tempvalue);
    }
    Serial.println("----------------");

    currentPosition = tempPosition;
    lastLayer = currentLayer;
    lcdUpdated = 1;

    /**
      Serial.println("Layer " + String(currentLayer + 1));
      Serial.println("Layer length " + String(currentLength));
      Serial.println("Layer position " + String(currentPosition));
    **/

  }
}


void currentValues() {

  Serial.println("Currentpress: " + currentPress);

  if (currentType == 1) {
    Serial.println("Currentvalue: " + String(value));
    Serial.println("Increments: " + String(numIncr));
    Serial.println("Min: " + String(Min));
    Serial.println("Max: " + String(Max));

  } else if (currentType == 2) {
    Serial.println("Currentvalue: " + options[value]);
    Serial.println("Options: ");

    for (int i = Min; i <= Max; i++) {
      Serial.println(options[i]);
    }
  }
}

void clearChar(int charPosition, int line) {
  lcd.setCursor (charPosition, line);
  lcd.print(" ");
}

void clearLine(int line) {
  lcd.setCursor (0, line);
  lcd.print("                    ");
}

void clearMenu() {
  lcd.setCursor (1, 0);
  lcd.print("                   ");
  lcd.setCursor (1, 1);
  lcd.print("                   ");
  lcd.setCursor (1, 2);
  lcd.print("                   ");
  lcd.setCursor (1, 3);
  lcd.print("                   ");
}

void clearSelect() {
  lcd.setCursor (0, 0);
  lcd.print(" ");
  lcd.setCursor (0, 1);
  lcd.print(" ");
  lcd.setCursor (0, 2);
  lcd.print(" ");
  lcd.setCursor (0, 3);
  lcd.print(" ");
}

void cursorSelect() {
  switch (currentSelect) {
    case 1:
      cursorLayer = 0;
      break;
    case 2:
      cursorLayer = 1;
      break;
    case 3:
      cursorLayer = 2;
      break;
    case 4:
      cursorLayer = 3;
      break;
    case 5:
      cursorLayer = 0;
      break;
    case 6:
      cursorLayer = 1;
      break;
    case 7:
      cursorLayer = 2;
      break;
    case 8:
      cursorLayer = 3;
      break;
  }
}

void writeCursor() {
  lcd.write(byte(0));
}


void clearValue() {
  for (int i = 20 - valueLength; i <= 20; i++) {
    lcd.setCursor (i, cursorLayer);
    lcd.print(" ");
  }
}
