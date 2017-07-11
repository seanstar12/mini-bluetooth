#include <SPI.h>
#include <Wire.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* RN-52 Bits for S%, 1EF6
  My bits: 001 1110 1111 0110 -> to HEX -> 0x1EF6
           Set with "S%,1EF6"

  [0] Bit 0  - Enable AVRCP
  [1] Bit 1  - Enable reconnect on power-on
  [1] Bit 2  - bluetooth discoverable on start
  [0] Bit 3  - Coded indicators 
  
  [1] Bit 4  - Reboot after disconnect
  [1] Bit 5  - Mute vol up/down tones
  [1] Bit 6  - Enable voice command on PIO4
  [1] Bit 7  - Disable system tones
  
  [0] Bit 8  - Power off after pairing timeout
  [1] Bit 9  - Reset after power off
  [1] Bit 10 - Enable list reconnect after panic
  [1] Bit 11 - Enable latch even indicator PIO2 (used for holding interrupt low)
  
  [1] Bit 12 - Enable track change event
  [0] Bit 13 - Enable tones playback at fixed vol
  [0] Bit 14 - Enable auto-accept
*/

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define DELTAY 2
#define HEART 23
#define SEN 22
#define LED 2

void setup()   {
  Serial.begin(38400);
  Serial2.begin(9600);
  Serial4.begin(38400);

  Serial4.println("Q");
  delay(100);

  pinMode(SEN, INPUT);
  pinMode(HEART, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(30, INPUT);

  // interrupt for rn-52 pin 3.
  attachInterrupt(30, change, FALLING);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  digitalWrite(LED, HIGH);
  digitalWrite(HEART, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  digitalWrite(HEART, LOW);

  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Initializing ...");
  display.display();
  Serial.println("My Ports are Ready");
}

String meta = "";

void loop() {
  _time = millis();
  char sequenceBytes[80];
  char checkSum;
  char serialByte;
  String resp;
  
  delay(1000);
  digitalWrite(23, digitalRead(30)); // 30 -> pin 3 from rn-52
  //if (!digitalRead(30)) {
  //  change();
  //}
}

void change() {
  long stat = getStatus();
  if (stat) {
    meta = getMetaData();
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(trackInfo("Title"));
    display.setCursor(0,24);
    display.print(trackInfo("Artist"));
    display.setCursor(0,36);
    display.print(trackInfo("Album"));
    display.display();
    
  }

  while (Serial4.available()) {
    char q = Serial4.read();
  }
}

String trackInfo(String type) {
  int n = meta.indexOf(type) + type.length() + 1;
  String _meta;

  if (n != -1) {
    _meta = meta.substring(n);
    n = _meta.indexOf('\n');
    _meta = _meta.substring(0, n);
  }
  else {
    _meta = "";
  }

  Serial.println(_meta);
  return _meta;
}

short getStatus() {
  short stat = 0;
  char c;
  
  while (Serial4.available()) {
    char q = Serial4.read();
  }
  delay(50);
  
  Serial4.println("Q");
  while(Serial4.available() == 0);
  
  while (c != '\r') {
    while (Serial4.available() == 0) {
      delay(5);
    }
    
    c = Serial4.read();

    if (c >= '0' && c <= '9') {
      stat *= 16;
      stat += (c - '0');
    }
    else if (c >= 'A' && c <= 'F') {
      stat *= 16;
      stat += (c - 'A') + 10;
    }
  }

  Serial.println(stat, HEX);
  return stat;
}

String getMetaData() {
  String _meta;
  int i = 6;
  long count;
  
  while (Serial4.available() > 0) {
    char q = Serial4.read();
  }
  delay(100);
  
  Serial4.println("AD");

  while(Serial4.available() == 0);

  while(i != 0) {
    if (Serial4.available() > 0) {
      char c = Serial4.read();
      count = millis();
      _meta += c;
      if (c == '\n') i--;
    }
    if ((millis() - count) > 500) i--;
  }

  meta = _meta;
  return meta;
}
