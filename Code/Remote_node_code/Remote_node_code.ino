#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <GyverButton.h>

#define BTN_PIN 2
#define sensorPin A1
#define FILTER_STEP 5
#define FILTER_COEF 0.05

const unsigned long period_time = 1500;
unsigned long nodeTimeout = 0;
GButton but(BTN_PIN, HIGH_PULL);
RF24 radio(9, 10);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 02;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t base00 = 00;
byte incomingData, data, isCivillian;
bool receivedFlag = false;

//Для фильтра
int val;
float val_f;
unsigned long filter_timer;
//
//Обработка датчика
bool sensorIsReady = true, shoot = false;
//

void setup() {
  // put your setup code here, to run once:
  SPI.begin();
  radio.begin();
  pinMode(8, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(sensorPin, INPUT);
  network.begin(90, this_node);  //(channel, node address)
  sensorTick();
  nodeTimeout = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update();
  but.tick();
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader headerIn;
    network.read(headerIn, &incomingData, sizeof(incomingData)); // Read the incoming data
    receivedFlag = true;
  }
  if (incomingData == 6 && receivedFlag) {
    digitalWrite(8, HIGH);
    digitalWrite(4, LOW);
    receivedFlag = false;
    isCivillian = false;
  } else if (incomingData == 7 && receivedFlag) {
    digitalWrite(4, HIGH);
    digitalWrite(8, LOW);
    receivedFlag = false;
    isCivillian = true;
  } else if (incomingData == 8) {
    digitalWrite(4, LOW);
    digitalWrite(8, LOW);
    receivedFlag = false;
  } else {
    receivedFlag = false;
  }
  sensorTick();
  //if (but.isClick()) {
  if (shoot) {
    shoot = false;
    if (isCivillian) {
      data = 7;
    } else {
      data = 6;
    }
    digitalWrite(4, LOW);
    digitalWrite(8, LOW);
    nodeTimeout = millis();
    RF24NetworkHeader header(base00);     // (Address where the data is going)
    bool ok = network.write(header, &data, sizeof(data)); // Send the data
  } 
  if (receivedFlag || millis() - nodeTimeout >= period_time) {
    RF24NetworkHeader header(base00);     // (Address where the data is going)
    data = 1;
    bool ok = network.write(header, &data, sizeof(data)); // Send the data
    nodeTimeout = millis();
    receivedFlag = false;
  }
}

void sensorTick() {
  if (millis() - filter_timer > FILTER_STEP) {
    filter_timer = millis();    // просто таймер
    // читаем значение (не обязательно с аналога, это может быть ЛЮБОЙ датчик)
    val = analogRead(sensorPin);
    // основной алгоритм фильтрации. Внимательно прокрутите его в голове, чтобы понять, как он работает
    val_f = val * FILTER_COEF + val_f * (1 - FILTER_COEF);
    // для примера выведем в порт
    Serial.println(val_f);
  }
  if (val_f < 1000 && sensorIsReady) {
    shoot = true;
    sensorIsReady = false;
  } else if (!sensorIsReady && val_f > 1000) {
    sensorIsReady = true;
  }
}
