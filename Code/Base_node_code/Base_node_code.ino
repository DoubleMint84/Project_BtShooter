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
unsigned long nodeTimeout = 0, raiseTimeout = 0, raisePeriod, shootTimeout;
GButton but(BTN_PIN, HIGH_PULL);
iarduino_OLED_txt oled(0x3C); 
extern uint8_t MediumFont[], SmallFont[];  
SoftwareSerial btSerial(2, 3); // RX, TX
RF24 radio(7, 9);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_base = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;
byte nodeState = 0;
byte incomingData, data;
byte dispUpdate = true, playFlag = false;
bool receivedFlag = false, isCivillian = false;

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
    Serial.print(incomingData);
    Serial.print(' ');
    Serial.println(nodeState);
  }
  nodeChecking();
  parsing();
  btCommands();
  if (gameMode == 1 && endTime <= millis() && playFlag) {
    playFlag = false;
    nodeState = 2;
    dispUpdate = true;
    data = 8;
    RF24NetworkHeader header(node01);     // (Address where the data is going)
    bool ok = network.write(header, &data, sizeof(data)); // Send the data
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
      calcTime();
      dispUpdate = true;
      
    } else if (intData[0] == 0) {
      digitalWrite(4, LOW);
      playFlag = false;
      data = 8;
      RF24NetworkHeader header(node01);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
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
    
    if (nodeState > 1) {
      oled.print(F("Online"), 0, 7);
    } else {
      oled.print(F("Offline"), 0, 7);
    }
    /*if (playFlag) {
      oled.print(F("ON"), OLED_R, 7);
      
    } else {
      oled.print(F("STOP"), OLED_R, 7);
    }*/
    oled.print(gameMode, OLED_R, 7);
    oled.setFont(MediumFont);
    if (gameMode == 1) {
      if (playFlag) {
        if (((endTime - millis()) / 1000) % 60 < 10){
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
  if (nodeState == 0) {
    nodeState = 1;
    data = 1;
    RF24NetworkHeader header(node01);     // (Address where the data is going)
    bool ok = network.write(header, &data, sizeof(data)); // Send the data
    //dispUpdate = true;
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
      //dispUpdate = true;
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
          hit++;
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
