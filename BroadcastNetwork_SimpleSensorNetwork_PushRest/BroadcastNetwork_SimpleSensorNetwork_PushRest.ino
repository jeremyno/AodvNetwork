/*
  Web Server bridge for a basic sensor network

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)
 * RF24 attached to 8,7, and SPI

 created 4/29/15
 by Jeremy Norman
 */

#include <SPI.h>
#include <Ethernet.h>
#include "AodvNetwork.h"
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <EEPROM.h>
#define DEBUG_ON
#include <SerialDebugOn.h>

#define PUSH_DEBUG

#define CE 8
#define CSN 7

#define FORWARD_GET "/scratch-webapp/sensor/put?"
#define FORWARD_EVERY 5000

IPAddress fserver(192,168,1,133);
int fport = 8080;


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x96, 0x54, 0x13, 0x6a, 0xd0, 0x34
};

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient client;
byte lastidx = 0;
long lastForward = 0;

BroadcastNetwork network(0);

class SensorValuePayload {
  public:
    byte sid;
    long sval;
};

class SensorValueEntry {
  public:
    byte mote;
    byte sid;
    long sval;
    long timestamp;
};

#define SensorValueEntryLogSize 20
SensorValueEntry SensorValueEntryTable[SensorValueEntryLogSize];
byte nextEntryIndex=0;

void forwardData(byte index) {
  client.stop();
  SensorValueEntry& ent = SensorValueEntryTable[index];

  long start = millis();
  
  if (client.connect(fserver,fport)) {
    client.print("GET ");
    client.print(FORWARD_GET);
    writeDataUrl(client);
    client.println(" HTTP/1.1");
    client.println("Host: 192.168.1.133");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
    
    #ifdef PUSH_DEBUG
    long e = millis() + 2000;
    
    while (e > millis()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
      }
    }
    debug("Took ");
    debugln(millis()-start);
    
    #endif
    
    lastidx=index;
    lastForward=millis();
  }
}

void setup() {
  // Open serial communications and wait for port to open:
  network.setAddr(EEPROM.read(0));

  debugBegin(9600);

  debugln(String("I am ")+network.getMyAddress());
  Mirf.spi = &MirfHardwareSpi;
  Mirf.cePin = CE;
  Mirf.csnPin = CSN;
  network.init();
  
  for(byte i = 0; i < SensorValueEntryLogSize; i++) {
    SensorValueEntryTable[i].mote = 0;
    SensorValueEntryTable[i].sid = 0;
    SensorValueEntryTable[i].sval = 0;
    SensorValueEntryTable[i].timestamp=0;
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac);
  debugln("pusher is at ");
  debugln(Ethernet.localIP());
}

void writeJson(EthernetClient& cli) {
  cli.println("{ entries : [");
  // output the value of each analog input pin  
  long now = millis();
  boolean comma = false;

  for (int i = 0; i < SensorValueEntryLogSize; i++) {
    SensorValueEntry& e = SensorValueEntryTable[i];
    if (e.mote == 0 && e.sid == 0) {
      continue;
    }
    if (comma) {
      cli.println(",");
    }
    cli.print(String("{mote:\"") + e.mote + "\", sensor:\""+e.sid+"\", value:\""+e.sval+"\", age:\""+(now - e.timestamp)+"\"}");
    comma = true;
  }
  cli.println();
  cli.println("]}");
}

void writeDataUrl(EthernetClient& cli) {
  // output the value of each analog input pin  
  boolean comma = false;
  cli.print(millis());
  cli.print("=");

  for (int i = 0; i < SensorValueEntryLogSize; i++) {
    SensorValueEntry& e = SensorValueEntryTable[i];
    if (e.mote == 0 && e.sid == 0) {
      continue;
    }
    if (comma) {
      cli.print("-");
    }
    cli.print(String("mote:") + e.mote + ".sensor:"+e.sid+".value:"+e.sval+".timestamp:"+e.timestamp);
    comma = true;
  }
}

void loop() {
  // listen for incoming clients  
  AodvPacket rpkt;
  
  while (network.getPacket(rpkt)) {   
    SensorValuePayload payload; 
    rpkt.readPayload((byte*)&payload,sizeof(SensorValuePayload));
    long ts = millis();
    
    bool found = false;
    byte idx;
        
    if (!found) {
      SensorValueEntryTable[nextEntryIndex].mote = rpkt.source;
      SensorValueEntryTable[nextEntryIndex].sid = payload.sid;
      SensorValueEntryTable[nextEntryIndex].sval = payload.sval;
      SensorValueEntryTable[nextEntryIndex].timestamp=ts;
      idx = nextEntryIndex;
      nextEntryIndex++;
      
      if (nextEntryIndex >= SensorValueEntryLogSize) {
        nextEntryIndex = 0;
      }  
    }
    
    if (lastidx == idx || (millis() - lastForward) > FORWARD_EVERY) {
      forwardData(idx);
    }
  }
}
