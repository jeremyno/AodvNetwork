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
//#define DEBUG_ON
#include <SerialDebugOn.h>

//#define PUSH_DEBUG

#define CE 8
#define CSN 7


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
byte lastidx = 0;

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

#define SensorValueEntryLogSize 40
SensorValueEntry SensorValueEntryTable[SensorValueEntryLogSize];
byte nextEntryIndex=0;

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
  Ethernet.begin(mac, ip);
  server.begin();
  debugln("server is at ");
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
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String tempLine = "";
    String getLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        tempLine += c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          
          if (getLine == "/" || getLine == "/index.htm") {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/json");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 5");  // refresh the page automatically every 5 sec
            client.println();
            writeJson(client);
          } else if (getLine.startsWith("/send?")) {
            getLine=getLine.substring(6);
            int mote = -1;
            int sensor = 0;
            long value = 0;
            while (getLine.length() > 0) {
              debugln(String("getLine: ") + getLine);
              int chunkIndex=getLine.indexOf("&");   
              if (chunkIndex < 0) {
                chunkIndex = getLine.length();
              }
              
              String chunk = getLine.substring(0,chunkIndex);
              debugln(String("Chunk: ") + chunk);
              int split = chunk.indexOf("=");
              if (split > 0) {
                String key = chunk.substring(0,split);
                long lValue = chunk.substring(split+1).toInt();
                
                if (key == "mote") {
                  mote = lValue;
                } else if (key == "sensor") {
                  sensor = lValue;
                } else if (key == "value") {
                  value = lValue;
                }                
              }
              getLine=getLine.substring(chunkIndex+1);
            }
            
            AodvPacket pkt = AodvPacket(0, mote, 0,  BROADCAST_ADDR, 0, 0, 0, NULL);

            SensorValuePayload payload; 
            payload.sid=sensor;
            payload.sval=value;
            pkt.setPayload((byte*)&payload,sizeof(SensorValuePayload));
            network.sendPacket(pkt);

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/json");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println();

            client.println(String("Sent... ") + mote+"/"+sensor+"/"+value);

          }
          // send a standard http response header
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          if (tempLine.startsWith("GET ")) {
            getLine = tempLine.substring(4);
            int idx = getLine.indexOf(" ");
            if (idx > 0) {
              getLine = getLine.substring(0,idx);
            }
          }
          tempLine = "";
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
  
  AodvPacket rpkt;
  
  while (network.getPacket(rpkt)) {   
    SensorValuePayload payload; 
    rpkt.readPayload((byte*)&payload,sizeof(SensorValuePayload));
    long ts = millis();
    
    bool found = false;
    byte idx;
    
    /*for(byte i = 0; i < SensorValueEntryLogSize && !found; i++) {
      SensorValueEntry& e = SensorValueEntryTable[i];
      if (e.mote == rpkt.source && e.sid == payload.sid) {
        e.sval = payload.sval;
        e.timestamp = ts;
        idx = i;
        found = true;
      }
    }*/
    
    if (! found) {
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
  }
}
