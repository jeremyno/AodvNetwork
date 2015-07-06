#include "AodvNetwork.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <EEPROM.h>
#define DEBUG_ON
#include <SerialDebugOn.h>
//#include <DebugOff.h>

#define CE 8
#define CSN 7

/**
 * This sketch uses an instance of BroadcastNetwork to receive any messages
 * from the sender. Change the address for every node in the network
 */

BroadcastNetwork network(0);

class SensorValuePayload {
  public:
    byte sid;
    long sval;
};

class SensorValueEntry {
  byte moteNumber;
  byte sensorNumber;
  long sensorValue;
  long timeStamp;
};

void setup(){
  network.setAddr(EEPROM.read(0));
  
  debugBegin(9600);
  debugln(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  Mirf.cePin = CE;
  Mirf.csnPin = CSN;
  network.init();
  
  //Mirf.configRegister( RF_SETUP,( 1<<RF_DR_LOW | 0<<RF_DR_HIGH ) | 1<<2 | 1<<1 );
}

void loop(){
  AodvPacket rpkt;
  
  if (network.getPacket(rpkt)) {   
    SensorValuePayload payload; 
    rpkt.readPayload((byte*)&payload,sizeof(SensorValuePayload));
    debugln(String("Sensor reading from ") + rpkt.source + " (via " + rpkt.lastHop + "), sensor #" + payload.sid + " has value " + payload.sval);
//    delay(250);
  }
}
