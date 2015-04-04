#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Set 1 to erase eeprom during troubleshooting, set 0 for operation.
#define ERASEEEPROM 0

// uncomment to get logging to serial.
//#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINTDEC(x)  Serial.print (x, DEC)
#define DEBUG_PRINTLN(x)   Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

// DHT pin and type
#define DHTPIN 8
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// OLED pin
#define OLED_MOSI   4
#define OLED_CLK   3
#define OLED_DC    6
#define OLED_CS    7
#define OLED_RESET 5
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Vars
float h, t, tt;
float maxt, mint, avgt;
float maxh, minh, avgh;
int px, i, tti, x;
int eeAddress = 0;
int eepromcount = 0;
int highbit = 16384;
int ifnotfull = -2;
int alternatedisplaycount = 0;
int alternatedisplayvalue = 0;
int storetoeepromtimer = 0;
int numsecbetweenwrites = 2;
unsigned long timer = 0;

void setup() {
  Serial.begin(57600);

  // Erase EEPROM
  if (ERASEEEPROM == 1) {
    for ( i = 0 ; i < EEPROM.length() ; i++ )
      EEPROM.write(i, 0);
    DEBUG_PRINT("EEPROM ERASED!  length = ");
    DEBUG_PRINTLN(i);
  }

  // Pins for the button
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(10, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);

  // Pins to drive the DHT-sensor
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  dht.begin();

  // Pins to drive the display
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC);
  display.setTextColor(WHITE);
  display.clearDisplay();

  // Get first value from EEPROM
  EEPROM.get(0, tti);
  DEBUG_PRINT("tti = ");
  DEBUG_PRINTLN(tti);

  // Check if the value of the first value is higher than 13000, set highbit
  if (tti > 13000) {
    highbit = 16384;
  } else {
    highbit = 0;
  }

  // Seldom used debugging.
  //highbit = 16384;

  DEBUG_PRINT("highbit = ");
  DEBUG_PRINTLN(highbit);
  DEBUG_PRINTLN("----");


  // Go thru EEPROM to check where to place next value
  for (i = 0; i < 256; i = i + 2) {
    EEPROM.get(i, tti);
    DEBUG_PRINT("tti: ");
    DEBUG_PRINTLN(tti);

    if (tti != 0) {
      DEBUG_PRINT("tti!=0 and i: ");
      DEBUG_PRINTLN(i);
      ifnotfull = i;
      if (highbit == 16384) {
        DEBUG_PRINTLN("highbit == 16384");
        if (tti < 13000) {
          DEBUG_PRINTLN("tti < 13000");
          eeAddress = i;
          i = 300;  // abort loop, search is over.
        }
      } else {
        DEBUG_PRINTLN("highbit != 16384");
        if (tti > 13000) {
          DEBUG_PRINTLN("tti > 13000");
          eeAddress = i;
          i = 300;  // abort loop, search is over.
        }
      }
    }
  }

  if (i < 300) {
    eeAddress = ifnotfull + 2;
  }

  DEBUG_PRINTLN("Loop done, eeAddress = ");

  DEBUG_PRINTLN(eeAddress);

  // This is the shortest delay to get working readings from the DHT sensor.
  delay(2000);

  DEBUG_PRINTLN("Setup done");

}


void loop() {

  h = dht.readHumidity();
  t = dht.readTemperature();

  DEBUG_PRINT("storetoeepromtimer: ");
  DEBUG_PRINTLN(storetoeepromtimer);

  // If the timer is 0, write values to EEPROM
  if (storetoeepromtimer <= 0) {
    EEPROM.put( eeAddress, int((t * 100) + highbit) );
    EEPROM.put( eeAddress + 256, int((h * 100) + highbit) );
  }

  avgt = 0;
  eepromcount = 0;
  for (i = 0; i < 256; i = i + 2) {
    EEPROM.get(i, tti);
    if (tti != 0) {
      if (tti > 13000) {
        tti = tti - 16384;
      }
      tt = float(float(float((tti)) / 100));
      if (maxt < tt) {
        maxt = tt;
      }
      if (mint > tt || mint == 0) {
        mint = tt;
      }
      avgt = avgt + tt;
      eepromcount = eepromcount + 1;
    }
  }

  avgh = 0;
  for (i = 256; i < 512; i = i + 2) {
    EEPROM.get(i, tti);
    if (tti != 0) {
      if (tti > 13000) {
        tti = tti - 16384;
      }
      tt = float(float(float((tti)) / 100));
      if (maxh < tt) {
        maxh = tt;
      }
      if (minh > tt || minh == 0) {
        minh = tt;
      }
      avgh = avgh + tt;
    }
  }

  avgt = avgt / eepromcount;
  avgh = avgh / eepromcount;

  if (alternatedisplaycount > 5) {
    alternatedisplaycount = 0;
    if (alternatedisplayvalue == 0) {
      alternatedisplayvalue = 256;
    } else {
      alternatedisplayvalue = 0;
    }
  }
  alternatedisplaycount += 1;

  DEBUG_PRINT("alternatedisplaycount: ");
  DEBUG_PRINT(alternatedisplaycount);
  DEBUG_PRINT("  alternatedisplayvalue: ");
  DEBUG_PRINTLN(alternatedisplayvalue);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 57);

  if (alternatedisplayvalue == 0) {
    display.print("t");
  } else {
    display.print("h");
  }

  x = 127;

  DEBUG_PRINT("-1st graphloop- eeAddress:");
  DEBUG_PRINTLN(eeAddress);
  // First batch of values..
  for (i = (eeAddress + alternatedisplayvalue); i >= alternatedisplayvalue; i = i - 2) {
    EEPROM.get(i, tti);
    DEBUG_PRINT("i:");
    DEBUG_PRINT(i);
    DEBUG_PRINT("  tti:");
    DEBUG_PRINT(tti);
    if (tti > 13000) {
      tti = tti - 16384;
    }
    if (tti != 0) {
      DEBUG_PRINT("  corrected_tti:");
      DEBUG_PRINT(tti);

      if (alternatedisplayvalue == 0) {
        px = map((tti), mint * 100, maxt * 100, 63, 33);
      } else {
        px = map((tti), minh * 100, maxh * 100, 63, 33);
      }

      display.drawPixel(x, px, WHITE);
    }
    DEBUG_PRINT(" x:");
    DEBUG_PRINTLN(x);
    x -= 1;
  }

  DEBUG_PRINTLN("-2nd graphloop-");

  // Second batch of values..
  for (i = (254 + alternatedisplayvalue); i > (eeAddress + alternatedisplayvalue); i = i - 2) {
    EEPROM.get(i, tti);
    DEBUG_PRINT("i:");
    DEBUG_PRINT(i);
    DEBUG_PRINT("  tti:");
    DEBUG_PRINT(tti);
    if (tti > 13000) {
      tti = tti - 16384;
    }
    if (tti != 0) {
      DEBUG_PRINT("  corrected_tti:");
      DEBUG_PRINT(tti);

      if (alternatedisplayvalue == 0) {
        px = map((tti), mint * 100, maxt * 100, 63, 33);
      } else {
        px = map((tti), minh * 100, maxh * 100, 63, 33);
      }

      display.drawPixel(x, px, WHITE);
    }
    DEBUG_PRINT(" x:");
    DEBUG_PRINTLN(x);
    x -= 1;
  }

  // Move to next address if the temp where stored to eeprom
  if (storetoeepromtimer <= 0) {
    eeAddress += 2;

    // Reset timer
    storetoeepromtimer = numsecbetweenwrites;
  }

  // Count the eeprom timer down, 2 seconds is the default delay.
  storetoeepromtimer -= 2;

  // If the end of the EEPROM is reached, start from 0 and change the highbit value.
  if (eeAddress >= 256) {
    eeAddress = 0;
    if (highbit == 16384) {
      highbit = 0;
    } else {
      highbit = 16384;
    }
  }

  // Print all the temps to the display
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print(t, 1);
  display.print(F("C"));
  display.setCursor(68, 0);
  display.print(h, 1);
  display.print(F("%"));
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.print(F("T>"));
  display.print(maxt, 1);
  display.print(F("C"));
  display.setCursor(48, 16);
  display.print(F("<"));
  display.print(mint, 1);
  display.print(F("C"));
  display.setCursor(90, 16);
  display.print(F("~"));
  display.print(avgt, 1);
  display.print(F("C"));
  display.setCursor(0, 24);
  display.print(F("H>"));
  display.print(maxh, 1);
  display.print(F("%"));
  display.setCursor(48, 24);
  display.print(F("<"));
  display.print(minh, 1);
  display.print(F("%"));
  display.setCursor(90, 24);
  display.print(F("~"));
  display.print(avgh, 1);
  display.print(F("%"));

  // Display everything in the buffer
  display.display();

  // This is the shortest delay to get working readings from the DHT sensor.

  timer = millis() + 2000;
  DEBUG_PRINTLN(timer);
  DEBUG_PRINTLN(millis());

  while (millis() < timer) {
    if (digitalRead(10) == LOW) {
      DEBUG_PRINT("Erasing EEPROM: ");
      eraseeeprom();
    }

    if (digitalRead(A1) == LOW) {
      DEBUG_PRINT("Change timescale: ");
      changetimescale();
    }

  }

  DEBUG_PRINTLN(millis());

}

void changetimescale() {

  unsigned long time;
  unsigned long timeleft;
  DEBUG_PRINTLN(time);
  DEBUG_PRINTLN(timeleft);

  while (digitalRead(A1) == LOW) {
    time = millis();
    timeleft = 2000;
    while ((digitalRead(A1) == LOW) && (timeleft > 0) && (timeleft < 50000)) {
      timeleft = 2000 - (millis() - time);
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print(F("Keep pressing to"));
      display.setCursor(0, 8);
      display.print(F("change saveinterval.."));
      display.setTextSize(1);
      display.setCursor(0, 16);

      switch (numsecbetweenwrites) {
        case 2:
          display.print(F("every 2 sec"));
          display.setCursor(0, 24);
          display.print(F("loglength 4,3 min"));
          break;
        case 10:
          display.print(F("every 10 sec"));
          display.setCursor(0, 24);
          display.print(F("loglength 21.3 min"));
          break;
        case 60:
          display.print(F("every 60 sec"));
          display.setCursor(0, 24);
          display.print(F("loglength: 2.1 h"));
          break;
        case 120:
          display.print(F("every 120 sec"));
          display.setCursor(0, 24);
          display.print(F("loglength: 4.2 h"));
          break;
        case 240:
          display.print(F("every 240 s (4 m)"));
          display.setCursor(0, 24);
          display.print(F("loglength: 8,5 h"));
          break;
        case 300:
          display.print(F("every 300 s (5 m)"));
          display.setCursor(0, 24);
          display.print(F("loglength: 10,6 h"));
          break;
        case 600:
          display.print(F("every 600 s (10 m)"));
          display.setCursor(0, 24);
          display.print(F("loglength: 21,2 h"));
          break;
      }

      display.display();

    }


    if ((timeleft <= 0) || (timeleft > 50000)) {
      switch (numsecbetweenwrites) {
        case 2:
          numsecbetweenwrites = 10;
          break;
        case 10:
          numsecbetweenwrites = 60;
          break;
        case 60:
          numsecbetweenwrites = 120;
          break;
        case 120:
          numsecbetweenwrites = 240;
          break;
        case 240:
          numsecbetweenwrites = 300;
          break;
        case 300:
          numsecbetweenwrites = 600;
          break;
        case 600:
          numsecbetweenwrites = 2;
          break;
      }
      storetoeepromtimer = numsecbetweenwrites;

    }

  }


}


void eraseeeprom() {

  unsigned long time = millis();
  unsigned long timeleft = 5000;
  DEBUG_PRINTLN(time);
  DEBUG_PRINTLN(timeleft);

  while ((digitalRead(10) == LOW) && (timeleft > 0) && (timeleft < 50000)) {
    timeleft = 5000 - (millis() - time);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(F("Keep pressing to"));
    display.setCursor(0, 8);
    display.print(F("erase log.."));
    display.setTextSize(3);
    display.setCursor(30, 16);
    display.print(timeleft);
    display.display();

  }

  if ((timeleft <= 0) || (timeleft > 50000)) {
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0, 16);
    display.print(F("ERASING"));
    display.display();

    for ( i = 0 ; i < EEPROM.length() ; i++ ) {
      EEPROM.write(i, 0);
      eeAddress = 0;
    }

    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0, 16);
    display.print(F("ERASED!"));
    display.display();
    delay(2000);
  }


}
