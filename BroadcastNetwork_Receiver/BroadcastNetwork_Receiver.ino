#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define MY_ADDR 0

/**
 * This sketch uses an instance of BroadcastNetwork to receive any messages
 * from the sender. Change the address for every node in the network
 */

BroadcastNetwork network(MY_ADDR);

void setup(){
  Serial.begin(9600);
  Serial.println(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  network.init();
}

void loop(){
  AodvPacket rpkt;
  
  if (network.getPacket(rpkt)) {    
    Serial.println("Packet returned!!!");
    long l;
    rpkt.readPayload((byte*)&l,sizeof(long));
    Serial.println(l);
    Serial.println("");
    delay(250);
  }
}
