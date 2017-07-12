#include <SPI.h>
#include <Wire.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "logo.h"


/* Notes
 *  Want to fix the method of pulling in settings.
 *  cleanup and synchronicity for pulling of data
 *  Need recovery for panic.
 *  Need interface for no BT connected
 *  Need settings for different screens
 *  Want inverse mode
 *  Battery breaks all the things
 *  Strange bugs when on independent power supply 
 *    crashes on disconnet of BT
 */
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define DELTAY 2
#define HEART 23
#define SEN 22
#define LED 2

String meta;
String settings;
bool changed = true;
int btStatus = 0;
int prevState = 0;
int battery = 0;

void setup() {
  delay(3000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  display.display();

  randomSeed(analogRead(A8));
  Serial.begin(38400);
  Serial4.begin(38400);
  Serial5.begin(38400);

  pinMode(3, INPUT);
  pinMode(7, INPUT);
  pinMode(12, INPUT);
  pinMode(24, OUTPUT);
  pinMode(35, INPUT);

  attachInterrupt(3, playPrev, FALLING);
  attachInterrupt(7, playNext, FALLING);
  attachInterrupt(12, setChanged, FALLING);
  attachInterrupt(35, blueInterr, FALLING);

  while (!Serial5);

  settings = getSettings();
  //battery = getBattery();
  battery = 80;
  drawLoad();
  setVolume();
  blueInterr();
}

void loop() {
  String msg;
  int _changed;
  char c;
  unsigned long _time = millis();

  if (changed){
    //btStatus = getStat();
    //battery = getBattery();
    changed = false;

    switch(btStatus & 0x00FF) {
      case 0x0003: // paused
        drawLogo();
        break;
      case 0x0000: // ?
      case 0x0001: // booting
        drawLoad();
        break;
      case 0x0002: // pairing
        animateBt();
        break;
      case 0x000D: // ad2p connected
      default:
        while(Serial5.available() > 0) {
          c = Serial5.read();
        }

        getMetaData();
        updateDisplay();
        break;
    }
  }
}

void blueInterr() {
  String msg;
  unsigned long _time = millis();

  while(Serial5.available() == 0 && millis() < _time + 500);
  
  if (Serial5.available()) {
    while(Serial5.available() > 0) {
      char c = Serial5.read();
      msg += c;
    }
    //Serial.println(msg);
  }

  prevState = btStatus;
  btStatus = getStat();
  changed = true;
}

int getStat() {
  int stat = 0;
  String msg;
  char c;
  int _time = millis();

  while(Serial5.available() > 0) {
    char q = Serial5.read();
  }

  _time = millis();
  Serial5.println("Q");
  while(Serial5.available() == 0 || millis() < _time + 300 );
  
  if (Serial5.available()) {
    while(Serial5.available() > 0 && c != '\r') {
      c = Serial5.read();
      msg += c;

      if (c >= '0' && c <= '9') {
        stat *= 16;
        stat += (c - '0');
      }
      else if (c >= 'A' && c <= 'F') {
        stat *= 16;
        stat += (c - 'A') + 10;
      }
    }
  }

  return stat;
}

void setChanged() {
  changed = true;
  //Serial.println(btStatus, HEX);
  //Serial.println("Bat: " + String(getBattery()));\
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(btStatus, HEX);
  display.display();
  delay(1000);
}

void drawLoad() {
  display.clearDisplay();
  display.drawBitmap(0, 0, img, 128, 64, 1);
  display.display();
}

void drawLogo() {
  display.clearDisplay();
  display.drawBitmap(20, 0, logoSmall, 128, 64, 1);
  display.setCursor(18, 52);
  display.setTextSize(1);
  display.print("Phone Connected");
  display.display();
}

void drawBt() {
  display.clearDisplay();
  display.drawBitmap(15, 5, mini, 32, 32, 1);
  display.drawBitmap(52, 5, bt, 32, 32, 1);
  
  display.setCursor(10, 40);
  display.setTextSize(2);
  display.print("Pin: 0123");
  display.display();
}

void animateBt() {
  int pos = 15;
  int off = 0;
  bool toggle = false;
  String pin = "Pin: " + parseInfo(settings, "PinCode");
  String btName = parseInfo(settings, "BTName");
  String mac = "MAC: " + parseInfo(settings, "BTA");
  
  while(btStatus == 0x01 || btStatus == 0x02) {
    display.clearDisplay();
    display.drawBitmap(pos, 5 + off, mini, 32, 32, 1);
    display.drawBitmap(pos + 50, 5, bt, 32, 32, 1);

    if (toggle) {
      display.setTextSize(1);
      display.setCursor(10, 40);
      display.print(btName + " " + pin);
      display.setCursor(10, 52);
      display.print(mac);
      display.display();
    }
    else {
      display.setCursor(10, 45);
      display.setTextSize(2);
      display.print(pin);
      display.display();
    }
    
    if (pos > display.width()) {
      toggle = !toggle;
      pos = - 75;
    }

    pos++;
    off = random(0, 3) - random(0, 2);
  }
}

// @TODO: investigate why delay is needed here.
void setVolume() {
  Serial5.println("SS,0E");
  delay(50);
  while(Serial5.available() == 0);

  while(Serial5.available() > 0) char c = Serial5.read();
}

void playNext() {
  cli();
  //Serial.println("Next Track");
  Serial5.println("AT+");
  delay(50);
  sei();
}

void playPrev() {
  cli();
  //Serial.println("Prev Track");
  Serial5.println("AT-");
  delay(50);
  sei();
}

String getMetaData() {
  String _meta;
  int i = 6;
  long count;
  
  Serial5.println("AD");
  while(Serial5.available() == 0);

  while(i != 0) {
    if (Serial5.available() > 0) {
      char c = Serial5.read();
      count = millis();
      _meta += c;
      if (c == '\n') i--;
    }
    if ((millis() - count) > 500) i--;
  }
  
  while(Serial5.available() > 0) {
    char c = Serial5.read();
  }

  meta = _meta;
  return meta;
}

String getSettings() {
  String _data;
  int i = 13;
  long count;
  int _time = millis();

  while(Serial5.available() == 0 && millis() < _time + 800);

  if (Serial5.available()) {
    while(Serial5.available() > 0) {
      char c = Serial5.read();
    }
  }

  Serial5.println("D");
  while(Serial5.available() == 0);

  while(i != 0 || millis() < _time + 800) {
    if (Serial5.available() > 0) {
      char c = Serial5.read();
      count = millis();
      _data += c;
      if (c == '\r') i--;
    }
  }
  
  while(Serial5.available() > 0) {
    char c = Serial5.read();
  }

  return _data;
}

int getBattery() {
  String level;
  char c;
  
  while (Serial5.available() > 0) c = Serial5.read();
  
  Serial5.println("GB");
  while (Serial5.available() == 0);

  while (c != '\r') {
    while (Serial5.available() == 0);
    
    c = Serial5.read();
    if (c != 0x25) level += c;
  }

  return parseInfo(level, "AGBatteryLevel").toInt();
}

void updateDisplay() {
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setCursor(3,5);
  display.print(trackInfo("Title"));
  display.setCursor(3,16);
  display.print(trackInfo("Artist"));
  display.setCursor(3,27);
  display.print(trackInfo("Album"));

  display.setCursor(18,54);
  display.fillRect(0, 52, display.width(), 12, 1);
  display.drawBitmap(2, 52, btBar, 12, 12, BLACK);
  //display.drawBitmap(108, 53, bat20, 32, 32, BLACK);
  //display.fillRect(109, 54, (battery/100.0)*15, 9, BLACK);
  display.setTextColor(BLACK);
  display.print("Audio");

  //display.setCursor(88,54);
  //display.print(String(battery) + "%");

  display.setTextColor(WHITE);
  display.display();
}

String trackInfo(String type) {
  int n = meta.indexOf(type) + type.length() + 1;
  String _meta;

  if (n != -1) {
    // from n
    _meta = meta.substring(n);
    n = _meta.indexOf('\n');
    _meta = _meta.substring(0, n);
  }
  else {
    _meta = "";
  }

  return _meta;
}

String parseInfo(String src, String type) {
  int n = src.indexOf(type) + type.length() + 1;
  String _data;

  if (n != -1) {
    _data = src.substring(n);
    n = _data.indexOf('\n');
    _data = _data.substring(0, n);
  }
  else {
    _data = "";
  }

  return _data;
}
