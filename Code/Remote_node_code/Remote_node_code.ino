#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <GyverButton.h>

#define BTN_PIN 2
const unsigned long period_time = 1500;
unsigned long nodeTimeout = 0;
GButton but(BTN_PIN, HIGH_PULL);
RF24 radio(9, 10);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t base00 = 00;
byte incomingData, data, isCivillian;
bool receivedFlag = false;

void setup() {
  // put your setup code here, to run once:
  SPI.begin();
  radio.begin();
  pinMode(8, OUTPUT);
  pinMode(4, OUTPUT);
  network.begin(90, this_node);  //(channel, node address)
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
  if (but.isClick()) {
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
