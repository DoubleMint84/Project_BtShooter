#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <GyverButton.h>
#include <GyverOLED.h>

#define BTN_PIN 4
#define PARSE_AMOUNT 5         // число значений в массиве, который хотим получить
int intData[PARSE_AMOUNT];     // массив численных значений после парсинга
String string_convert = "";
boolean btFlag;
boolean getStarted;
byte index;
const unsigned long period_time = 3000, shootPeriod = 5000 ;
unsigned long nodeTimeout = 0, raiseTimeout = 0, raisePeriod, shootTimeout;
GButton but(BTN_PIN, HIGH_PULL);
GyverOLED oled;
SoftwareSerial btSerial(2, 3); // RX, TX
RF24 radio(7, 9);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_base = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;
byte nodeState = 0;
byte incomingData, data;
byte dispUpdate = false, playFlag = false;
bool receivedFlag = false, isCivillian = false;
int wrongHit = 0, miss = 0;

void setup() {
  unsigned long seed;
  for (int i = 0; i < 400; i++) {
    seed = 1;
    for (byte j = 0; j < 16; j++) {
      seed *= 4;
      seed += analogRead(A0) & 3;
    }
    Serial.println(seed);
  }
  SPI.begin();
  Serial.begin(9600);
  btSerial.begin(9600);
  radio.begin();
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  network.begin(90, this_base);  //(channel, node address)
  oled.init(OLED128x64, 800);
  oled.setContrast(1);
  oled.clear();      // очистить
  oled.home();      // курсор в 0,0
  oled.scale1X();    // масштаб шрифта х1
  oled.print("Привет, ");
  oled.update();
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update();
  but.tick();
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    receivedFlag = true;
    Serial.print(incomingData);
    Serial.print(' ');
    Serial.println(nodeState);
  }
  if (nodeState == 0) {
    nodeState = 1;
    data = 1;
    RF24NetworkHeader header(node01);     // (Address where the data is going)
    bool ok = network.write(header, &data, sizeof(data)); // Send the data
    dispUpdate = true;
    nodeTimeout = millis();
    Serial.println("CASE 0");
  } else if (nodeState == 1) {
    if (receivedFlag) {
      Serial.println("received");
      nodeState = 2;
      dispUpdate = true;
      nodeTimeout = millis();
      digitalWrite(5, HIGH);
      receivedFlag = false;
      calcTime();
    } else if (millis() - nodeTimeout >= period_time) {
      nodeState = 0;
      digitalWrite(5, LOW);
      digitalWrite(4, LOW);
      dispUpdate = true;
    }
    Serial.println("CASE 1");
  } else if (nodeState == 2) {
    if (receivedFlag) {
      nodeTimeout = millis();
      receivedFlag = false;
    } else if (millis() - nodeTimeout >= period_time) {
      nodeState = 0;
      digitalWrite(5, LOW);
      digitalWrite(4, LOW);
      dispUpdate = true;
    }
    if (millis() - raiseTimeout >= raisePeriod && nodeState == 2 && playFlag) {
      if (isCivillian) {
        nodeState = 4;
        data = 7;
      } else {
        nodeState = 3;
        data = 6;
      }
      shootTimeout = millis();
      RF24NetworkHeader header(node01);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
      dispUpdate = true;
    }
    // Serial.println("CASE 2");
  } else if (nodeState == 3) {
    if (playFlag) {
      if (receivedFlag) {
        if (incomingData == 6) {

          nodeState = 2;
          calcTime();
          dispUpdate = true;
        }
        nodeTimeout = millis();
        receivedFlag = false;
      } else if (millis() - nodeTimeout >= period_time) {
        nodeState = 0;
        digitalWrite(5, LOW);
        digitalWrite(4, LOW);
        dispUpdate = true;
      }
      if (millis() - shootTimeout >= shootPeriod) {
        nodeState = 2;
        miss++;
        data = 8;
        RF24NetworkHeader header(node01);     // (Address where the data is going)
        bool ok = network.write(header, &data, sizeof(data)); // Send the data
        calcTime();
        dispUpdate = true;
      }
    } else {
      nodeState = 2;
      dispUpdate = true;
    }
  } else if (nodeState == 4) {
    if (playFlag) {
      if (receivedFlag) {
        if (incomingData == 7) {
          wrongHit++;
          nodeState = 2;
          calcTime();
          dispUpdate = true;
        }
        nodeTimeout = millis();
        receivedFlag = false;
      } else if (millis() - nodeTimeout >= period_time) {
        nodeState = 0;
        digitalWrite(5, LOW);
        digitalWrite(4, LOW);
        dispUpdate = true;
      }
      if (millis() - shootTimeout >= shootPeriod) {
        nodeState = 2;
        data = 8;
        RF24NetworkHeader header(node01);     // (Address where the data is going)
        bool ok = network.write(header, &data, sizeof(data)); // Send the data
        calcTime();
        dispUpdate = true;
      }
    } else {
      nodeState = 2;
      dispUpdate = true;
    }
  }
  parsing();
  if (btFlag) {                           // если получены данные
    btFlag = false;
    if (intData[0] == 1) {
      playFlag = true;
      digitalWrite(4, HIGH);
      calcTime();
      dispUpdate = true;
      
    } else if (intData[0] == 0) {
      digitalWrite(4, LOW);
      playFlag = false;
      data = 8;
      RF24NetworkHeader header(node01);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
      dispUpdate = true;
    }
  }
  if (dispUpdate) {
    oled.clear();      // очистить
    oled.home();      // курсор в 0,0
    oled.scale1X();    // масштаб шрифта х1
    oled.print(nodeState);
    oled.print(' ');
    oled.print(wrongHit);
    oled.print(' ');
    oled.print(miss);
    oled.print(' ');
    if (playFlag) {
      oled.print("Playing");
      
    } else {
      oled.print("Stopped");
    }

    oled.update();
    dispUpdate = false;
  }

}

void calcTime() {
  raisePeriod = random(1000, 10000);
  isCivillian = !random(4);
  raiseTimeout = millis();
}

void parsing() {
  if (btSerial.available() > 0) {
    char incomingByte = btSerial.read();        // обязательно ЧИТАЕМ входящий символ
    if (getStarted) {                         // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != ' ' && incomingByte != ';') {   // если это не пробел И не конец
        string_convert += incomingByte;       // складываем в строку
      } else {                                // если это пробел или ; конец пакета
        intData[index] = string_convert.toInt();  // преобразуем строку в int и кладём в массив
        string_convert = "";                  // очищаем строку
        index++;                              // переходим к парсингу следующего элемента массива
      }
    }
    if (incomingByte == '$') {                // если это $
      getStarted = true;                      // поднимаем флаг, что можно парсить
      index = 0;                              // сбрасываем индекс
      string_convert = "";                    // очищаем строку
    }
    if (incomingByte == ';') {                // если таки приняли ; - конец парсинга
      getStarted = false;                     // сброс
      btFlag = true;                    // флаг на принятие
    }
  }
}
