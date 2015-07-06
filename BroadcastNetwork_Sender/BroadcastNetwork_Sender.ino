#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
//#include <MirfUtils.h>

#define UPDATE_TIME 10000
//#define DEBUG_ON
#include <SerialDebugOn.h>

/**
 * This sketch broadcasts information to all of the BroadcastNetwork
 * Nodes in range. Those nodes, in turn, repeat the message to 
 * all in range of them.
 */
BroadcastNetwork network(1);

void setup(){
  debugBegin(9600);
  debugln(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;  
  network.init();
  network.setRFPwrLevel(0);
}
  
void loop(){
  AodvPacket pkt = AodvPacket(3, AODV_CMD_DATA);
  long time = millis();
  pkt.setPayload(time);

  network.sendPacket(pkt);
  debugln(String("Sent ") + time);
  
  
  while (millis() - time < UPDATE_TIME) {
    network.getPacket(pkt);
  }
}
