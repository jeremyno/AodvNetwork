#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <MirfUtils.h>

/**
 * This sketch uses an instance of BroadcastNetwork to receive any messages
 * from the sender. Change the address for every node in the network
 */

BroadcastNetwork network(3);

void setup(){
  Serial.begin(9600);
  Serial.println(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  network.init();
  //setRFPwrLevel(0);
}

long lastPkt = 0;
AodvPacket rpkt;

void loop(){
  if (network.getPacket(rpkt)) {
    Serial.println("Packet returned!!!");
    long l;
    rpkt.readPayload(l);
    Serial.println(l);
    
    l = l / 1000;
    
    AodvPacket toNext(2,AODV_CMD_DATA);
    toNext.setPayload(l);
    network.sendPacket(toNext);
    Serial.println("");
  }
}
