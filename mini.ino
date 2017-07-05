#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define HEART 23
#define SEN 22
#define LED 2

void setup()   {
  Serial2.begin(9600); // using Serial2 from teensy3.6 pins {rx:9, tx:10}

  pinMode(SEN, INPUT);
  pinMode(HEART, OUTPUT);
  pinMode(LED, OUTPUT);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // 128x64 oled screen
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  digitalWrite(LED, HIGH);
  digitalWrite(HEART, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  digitalWrite(HEART, LOW);

  display.clearDisplay();
  display.display();
}

int count = 0;
int frame = 0;
unsigned short refreshDisplay = 0;
String cmd = "None";
char lastByte;
String carBuffer;
String carLast;
String seqBuff = "";
unsigned long _time = millis();
unsigned long _prevTime = _time;

void loop() {
  _time = millis();
  char sequenceBytes[80];
  char checkSum;
  char serialByte;

  if (!digitalRead(SEN)) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
  
  if (Serial2.available() > 0) {
    count++;
    digitalWrite(HEART, HIGH);
    serialByte = Serial2.read();

    sequenceBytes[0] = serialByte;
    
    if (serialByte == 80) {
      digitalWrite(HEART, HIGH);
      serialByte = Serial2.read();
      sequenceBytes[1] = serialByte;

      if ((serialByte == 4) || (serialByte == 3)) {
        do { } while (Serial2.available() == 0);
        checkSum = Serial2.read();
        sequenceBytes[2] = checkSum;
        
        do { } while (Serial2.available() == 0);
        checkSum += Serial2.read();
        sequenceBytes[3] = checkSum;
        
        do { } while (Serial2.available() == 0);
        checkSum -= Serial2.read();
        sequenceBytes[4] = checkSum;

        switch (checkSum) {
          case 137:
            cmd = "Vol Up";
            break;
          case 138:
            cmd = "Vol Dn";
            break;
          case 162:
            cmd = "Rt Dn";
            break;
          case 130:
            cmd = "Rt Up";
            break;
          case 146:
            cmd = "Rt Hold";
            break;
          case 155:
            cmd = "Lt Dn";
            break;
          case 123:
            cmd = "Lt Up";
            break;
          case 139:
            cmd = "Lt Hold";
            break;
          case 131:
            cmd = "Tel Dn";
            break;
          case 99:
            cmd = "Tel Up";
            break;
          case 115:
            cmd = "Tel Hold";
            break;
          case 47:
            cmd = "R/t";
            break;
        }

        count++;
        digitalWrite(HEART, LOW);
      }
    }
  }

  if (_time > _prevTime + 100) {
    frame++;
    updateDisplay(count, frame, cmd, sequenceBytes);
    _prevTime = _time;

    if (frame > 8) {
      frame = 0;
    }
  }
}

void updateDisplay(int count, int frame, String cmd, char* sequenceBytes) {
  display.clearDisplay();
  printCount(count);
  printFrame(frame);
  printByte(sequenceBytes[0]);
  printStatus("Action: " + cmd, 12);
  printBuffer(sequenceBytes);
  display.display();
}

void printCount(int count) {
  display.setCursor(0,0);

  display.println("Msgs: " + String(count));
}
void printFrame(int frame) {
  display.setCursor(110,0);
  display.println(String(frame));
}

void printBuffer(char* buff) {
  display.setCursor(0,36);

  for (unsigned int i=0; i < sizeof(buff); i++) {
    display.print(buff[i], HEX);
  }

  display.setCursor(0, 48);
  display.print(buff[0], HEX);
  display.print(" ");
  display.print(buff[1], HEX);
  display.print(" ");
  display.print(buff[2], HEX);
  display.print(" ");
  display.print(buff[3], HEX);
  display.print(" ");
  display.print(buff[4], HEX);

}

void printByte(char buff) {
  display.setCursor(110,36);

  display.print(buff, HEX);
}

void printByteString(String buff) {
  display.setCursor(0,36);

  display.println("CMD: " + buff);
}

void printStatus(String msg, uint16_t y) {
  display.setCursor(0,y);

  display.println(msg);
}
