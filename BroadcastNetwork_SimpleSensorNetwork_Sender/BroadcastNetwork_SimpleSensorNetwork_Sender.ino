#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <EEPROM.h>
//#include <SerialDebugOn.h>
#include <DebugOff.h>

#define CE 8
#define CSN 7
#define READ_DELAY 3000

/**
 * This sketch broadcasts information to all of the BroadcastNetwork
 * Nodes in range. Those nodes, in turn, repeat the message to 
 * all in range of them.
 */
BroadcastNetwork network(0);

void setup(){
  network.myaddr = EEPROM.read(0);

  debugBegin(9600);
  debugln(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  Mirf.cePin = CE;
  Mirf.csnPin = CSN;
  network.init();
}

class SensorValuePayload {
  public:
    byte sid;
    long sval;
};

void loop(){
  AodvPacket pkt = AodvPacket(0, BROADCAST_ADDR, 0,  BROADCAST_ADDR, 0, 0, 0, NULL);

  for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel);
    SensorValuePayload payload;
    payload.sid=analogChannel;
    payload.sval=sensorReading;
    pkt.setPayload((byte*)&payload,sizeof(SensorValuePayload));
    network.sendPacket(pkt);
    delay(random(10));
  }
  
  long start = millis();
  
  while (millis()-start < READ_DELAY) {
    if (network.getPacket(pkt) && pkt.destination == network.myaddr) {
      SensorValuePayload payload;
      pkt.readPayload((byte*)&payload,sizeof(SensorValuePayload));
      debug("RECV ");
      debug(payload.sid);
      debug("/");
      debugln(payload.sval);
      if (payload.sid >=3 && payload.sid <=5) {
        digitalWrite(payload.sid,payload.sval);
      }
    }
  }
}
