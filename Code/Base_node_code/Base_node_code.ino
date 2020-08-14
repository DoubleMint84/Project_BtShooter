#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <GyverButton.h>
#include <iarduino_OLED_txt.h>

#define BTN_PIN 4
#define PARSE_AMOUNT 5         // число значений в массиве, который хотим получить
int intData[PARSE_AMOUNT];     // массив численных значений после парсинга
String string_convert = "";
boolean btFlag;
boolean getStarted;
byte index;
const unsigned long period_time = 3000, shootPeriod = 5000 ;
unsigned long nodeTimeout[] = {0, 0, 0, 0, 0};
unsigned long raiseTimeout[] = {0, 0, 0, 0, 0};
unsigned long raisePeriod[] = {0, 0, 0, 0, 0};
unsigned long shootTimeout[] = {0, 0, 0, 0, 0};
bool isCivillian[] = {false, false, false, false, false};
GButton but(BTN_PIN, HIGH_PULL);
iarduino_OLED_txt oled(0x3C);
extern uint8_t MediumFont[], SmallFont[];
SoftwareSerial btSerial(2, 3); // RX, TX
RF24 radio(7, 9);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_base = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t nodes[] = {1, 2, 3, 4, 5};
uint16_t fromNode;
byte nodeState[] = {0, 0, 0, 0, 0};
byte incomingData, data;
byte dispUpdate = true, playFlag = false;
bool receivedFlag = false;


//Игровые переменные - СТАТИСТИКА
int wrongHit = 0, miss = 0, hit = 0;
//
//Игровые переменные - РЕЖИМ И ПАРАМЕТРЫ ИГРЫ
int gameMode = 1, gameTimeMin = 1, gameTimeSec = 30, remainHP = 100;
int gameHP = 100, gameMinDamage = 5, gameMaxDamage = 30;
unsigned long surviveTime = 0, startTime = 0, endTime = 0, timeRemain = 90000, oledUpdTimer = 0;
//
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
  oled.begin();
  oled.setFont(SmallFont);
  oled.print("Hello", OLED_C, 4);
  delay(2000);
  oled.clrScr();
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update();
  but.tick();
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    receivedFlag = true;
    fromNode = header.from_node;
    Serial.print(incomingData);
    Serial.print(' ');
    
     Serial.println( header.from_node== 02 ? "1" : "0");
     Serial.print(' ');
    
     Serial.println( nodeState[1]);
  }
  nodeChecking();
  parsing();
  btCommands();
  if (gameMode == 1 && endTime <= millis() && playFlag) {
    playFlag = false;
    data = 8;
    for (int i = 0; i < 5; i++) {
      if (nodeState[i] >= 2) {
        nodeState[i] = 2;
      }
      RF24NetworkHeader header(nodes[i]);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
    }
    dispUpdate = true;
    digitalWrite(4, LOW);
  }
  if (gameMode == 1 && playFlag && millis() - oledUpdTimer >= 1000) {
    dispUpdate = true;
    oledUpdTimer = millis();
  }
  oledDisplay();
}

void btCommands () {
  if (btFlag) {                           // если получены данные
    btFlag = false;
    if (intData[0] == 1) {
      playFlag = true;
      digitalWrite(4, HIGH);
      if (gameMode == 1) {
        endTime = millis() + (long)gameTimeMin * 60 * 1000 + (long)gameTimeSec * 1000;
      } else if (gameMode == 2) {
        remainHP = gameHP;
        startTime = millis();
      }
      oledUpdTimer = millis();
      for (int i = 0; i < 5; i++) {
        calcTime(i);
      }
      
      dispUpdate = true;

    } else if (intData[0] == 0) {
      digitalWrite(4, LOW);
      playFlag = false;
      data = 8;
      for (int i = 0; i < 5; i++) {
        RF24NetworkHeader header(nodes);     // (Address where the data is going)
        bool ok = network.write(header, &data, sizeof(data)); // Send the data
      }
      dispUpdate = true;
    } else if (intData[0] == 2) {
      if (intData[1] == 1) {
        gameMode = 1; // Игровой режим - на время
        gameTimeMin = intData[2];
        gameTimeSec = intData[3];
        dispUpdate = true;
      } else if (intData[1] == 2) {
        gameMode = 2; // Игровой режим - выживание
        gameHP = intData[2];
        gameMinDamage = intData[3];
        gameMaxDamage = intData[4];
        dispUpdate = true;
      }
    }
  }
}

void oledDisplay() {
  if (dispUpdate) {
    oled.clrScr();      // очистить
    // курсор в 0,0
    oled.setFont(SmallFont);    // масштаб шрифта х1
    //oled.print(nodeState);
    //oled.print(' ');
    oled.print(String(hit) + " " + String(wrongHit) + " " + String(miss),    0,      0);
    String stat = "";
    for (int i = 0; i < 5; i++) {
        
      stat += String(nodeState[i]);
    }
    oled.print(stat, 0, 7);
    /*if (playFlag) {
      oled.print(F("ON"), OLED_R, 7);

      } else {
      oled.print(F("STOP"), OLED_R, 7);
      }*/
    oled.print(gameMode, OLED_R, 7);
    oled.setFont(MediumFont);
    if (gameMode == 1) {
      if (playFlag) {
        if (((endTime - millis()) / 1000) % 60 < 10) {
          oled.print(String(((endTime - millis()) / 1000) / 60) + ":0" + String(((endTime - millis()) / 1000) % 60), OLED_C, 4);
        } else {
          oled.print(String(((endTime - millis()) / 1000) / 60) + ":" + String(((endTime - millis()) / 1000) % 60), OLED_C, 4);
        }
      } else {
        oled.print(String(gameTimeMin) + ":" + String(gameTimeSec), OLED_C, 4);
      }

    } else if (gameMode == 2) {
      if (playFlag) {
        oled.print(String(remainHP) + " HP", OLED_C, 2);
      } else {
        oled.print(String(gameHP) + " HP", OLED_C, 3);
        oled.print(F("0:00"), OLED_C, 5);
      }
    }


    dispUpdate = false;
  }
}

void nodeChecking() {
  for (int i = 0; i < 5; i++) {
    if (nodeState[i] < 2 && playFlag) {
      continue;
    }
    if (nodeState[i] == 0) {
      nodeState[i] = 1;
      data = 1;
      RF24NetworkHeader header(nodes[i]);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
      dispUpdate = true;
      nodeTimeout[i] = millis();
      //Serial.println("CASE 0");
    } else if (nodeState[i] == 1) {
      if (receivedFlag && fromNode == nodes[i]) {
        Serial.println("received");
        nodeState[i] = 2;
        dispUpdate = true;
        nodeTimeout[i] = millis();
        digitalWrite(5, HIGH);
        receivedFlag = false;
        calcTime(i);
      } else if (millis() - nodeTimeout[i] >= period_time) {
        nodeState[i] = 0;
        digitalWrite(5, LOW);
        digitalWrite(4, LOW);
        dispUpdate = true;
      }
      //Serial.println("CASE 1");
    } else if (nodeState[i] == 2) {
      if (receivedFlag && fromNode == nodes[i]) {
        nodeTimeout[i] = millis();
        receivedFlag = false;
      } else if (millis() - nodeTimeout[i] >= period_time) {
        nodeState[i] = 0;
        digitalWrite(5, LOW);
        digitalWrite(4, LOW);
        dispUpdate = true;
      }
      if (millis() - raiseTimeout[i] >= raisePeriod[i] && nodeState[i] == 2 && playFlag) {
        if (isCivillian[i]) {
          nodeState[i] = 4;
          data = 7;
        } else {
          nodeState[i] = 3;
          data = 6;
        }
        shootTimeout[i] = millis();
        RF24NetworkHeader header(nodes[i]);     // (Address where the data is going)
        bool ok = network.write(header, &data, sizeof(data)); // Send the data
        dispUpdate = true;
      }
      // Serial.println("CASE 2");
    } else if (nodeState[i] == 3) {
      if (playFlag) {
        if (receivedFlag && fromNode == nodes[i]) {
          if (incomingData == 6) {
            hit++;
            nodeState[i] = 2;
            calcTime(i);
            dispUpdate = true;
          }
          nodeTimeout[i] = millis();
          receivedFlag = false;
        } else if (millis() - nodeTimeout[i] >= period_time) {
          nodeState[i] = 0;
          digitalWrite(5, LOW);
          digitalWrite(4, LOW);
          dispUpdate = true;
        }
        if (millis() - shootTimeout[i] >= shootPeriod) {
          nodeState[i] = 2;
          miss++;
          data = 8;
          RF24NetworkHeader header(nodes[i]);     // (Address where the data is going)
          bool ok = network.write(header, &data, sizeof(data)); // Send the data
          calcTime(i);
          dispUpdate = true;
        }
      } else {
        nodeState[i] = 2;
        dispUpdate = true;
      }
    } else if (nodeState[i] == 4) {
      if (playFlag) {
        if (receivedFlag && fromNode == nodes[i]) {
          if (incomingData == 7) {
            wrongHit++;
            nodeState[i] = 2;
            calcTime(i);
            dispUpdate = true;
          }
          nodeTimeout[i] = millis();
          receivedFlag = false;
        } else if (millis() - nodeTimeout[i] >= period_time) {
          nodeState[i] = 0;
          digitalWrite(5, LOW);
          digitalWrite(4, LOW);
          dispUpdate = true;
        }
        if (millis() - shootTimeout[i] >= shootPeriod) {
          nodeState[i] = 2;
          data = 8;
          RF24NetworkHeader header(nodes[i]);     // (Address where the data is going)
          bool ok = network.write(header, &data, sizeof(data)); // Send the data
          calcTime(i);
          dispUpdate = true;
        }
      } else {
        nodeState[i] = 2;
        dispUpdate = true;
      }
    }
  }

}

void calcTime(int tnode) {
  raisePeriod[tnode] = random(1000, 10000);
  isCivillian[tnode] = !random(4);
  raiseTimeout[tnode] = millis();
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
