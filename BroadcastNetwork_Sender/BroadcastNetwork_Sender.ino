#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

/**
 * This sketch broadcasts information to all of the BroadcastNetwork
 * Nodes in range. Those nodes, in turn, repeat the message to 
 * all in range of them.
 */
BroadcastNetwork network(1);

void setup(){
  Serial.begin(9600);
  Serial.println(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  network.init();
}

void loop(){
  AodvPacket pkt = AodvPacket(0, BROADCAST_ADDR, 0,  BROADCAST_ADDR, 0, 0, 0, NULL);
  long time = millis();
  pkt.setPayload((byte*)&time,sizeof(long));

  network.sendPacket(pkt);
  delay(3000);
}
