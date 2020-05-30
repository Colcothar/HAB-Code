#include <SPI.h>
#include <LoRa.h>

//define the pins used by the transceiver module
#define ss 10
#define rst 9
#define dio0 5

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  //while (!Serial);
  Serial.println("LoRa Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  LoRa.setTxPower(20);
  LoRa.setSignalBandwidth(7.8E3);
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(8);
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print("Packet ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;

  delay(10000);
}
