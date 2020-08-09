#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <GyverButton.h>

#define BTN_PIN 2
const unsigned long period_time = 3000;
unsigned long nodeTimeout = 0;
GButton but(BTN_PIN, HIGH_PULL);
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t base00 = 00;
byte incomingData, data;

void setup() {
  // put your setup code here, to run once:
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update();
  but.tick();
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    if (incomingData == 1) {
      RF24NetworkHeader header(base00);     // (Address where the data is going)
      data = 1;
      bool ok = network.write(header, &data, sizeof(data)); // Send the data
    }
  }
}
