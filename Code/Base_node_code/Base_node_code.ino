#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <GyverButton.h>
#include <GyverOLED.h>

#define BTN_PIN 4
const unsigned long period_time = 3000;
unsigned long nodeTimeout = 0;
GButton but(BTN_PIN, HIGH_PULL);
GyverOLED oled;
SoftwareSerial btSerial(3, 2); // RX, TX
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_base = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;
byte nodeState = 0;
byte incomingData, data;
byte dispUpdate = false;
bool receivedFlag = false;

void setup() {
  SPI.begin();
  btSerial.begin(9600);
  radio.begin();
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
  }
  switch (nodeState) {
    case 0:
      nodeState = 1;
      data = 1;
      RF24NetworkHeader header(node01);     // (Address where the data is going)
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
      dispUpdate = true;
      break;
    case 1:
      if (receivedFlag) {
        if (data == 1) {
          nodeState = 2;
          dispUpdate = true;
          nodeTimeout = millis();
        } 
        receivedFlag = false;
              
      }
      break;
    case 2:
      if (receivedFlag) {
        nodeTimeout = millis();
        receivedFlag = false;
      } else if (millis() - nodeTimeout >= period_time) {
        nodeState = 0;
        dispUpdate = true;
      }
      break;
  }
  if (dispUpdate) {
    oled.clear();      // очистить
    oled.home();      // курсор в 0,0
    oled.scale1X();    // масштаб шрифта х1  
    oled.print(nodeState);
    oled.update();
  }
  
}
