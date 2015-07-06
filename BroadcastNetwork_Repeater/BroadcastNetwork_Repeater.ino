#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

/**
 * This sketch uses an instance of BroadcastNetwork to receive any messages
 * from the sender. Change the address for every node in the network
 */

BroadcastNetwork network(2);

void setup(){
  Serial.begin(9600);
  Serial.println(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  network.init();
}

long lastPkt = 0;
AodvPacket rpkt;

void loop(){
  if (network.getPacket(rpkt)) {
    Serial.println("");
    Serial.println("");

    Serial.println("Packet returned!!!");
    long l;
    rpkt.readPayload(l);
    Serial.println(l);
    Serial.println("");
    Serial.println("");
  }
}
