#ifndef AODV_UDP_PROTO_H
#include "AodvNetwork.h"
#include "Arduino.h"

#define UDP_PAYLOAD_SIZE PAYLOAD_SIZE-2

class UdpPayload {
  public:
    UdpPayload() : source(0), destination(0) {}
    UdpPayload(byte sourcePort, byte destinationPort) 
      : source(sourcePort), destination(destinationPort)
      {};
      
    void setPayload(byte* data, byte len);
    void readPayload(byte* data, byte len);

    template<class T> void setPayload(T& ref) {
      setPayload((byte*)&ref,sizeof(T));
    }
    
    template<class T> void readPayload(T& ref) {
      readPayload((byte*)&ref,sizeof(T));
    }
  private:
    byte source;
    byte destination;
    byte payload[UDP_PAYLOAD_SIZE];
};

template<class T>
AodvPacket UdpPacket(BroadcastNetwork& network, byte destination, byte sPort, byte dPort, T& data) {
  AodvPacket pkt = AodvPacket(destination,AODV_CMD_PROTO_UDP);
  UdpPayload udp = UdpPayload(sPort,dPort);
  udp.setPayload(data);
  pkt.setPayload(udp);
  
  return pkt;
}

#endif