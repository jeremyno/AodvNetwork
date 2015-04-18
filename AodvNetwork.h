/** 
 * Copyright (c) Jeremy Norman. All right reserved.
 *
 * This library is free software; you can redistribute it and/or modify it under the terms of the GNU 
 * Lesser General Public License as published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library; 
 * if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 
 * 02110-1301 USA
 */
#include "arduino.h"
#include <Mirf.h>

#define MAX_ROUTES 10
#define MAX_HOPS 5
#define BROADCAST_MEMORY 10
#define MAX_PACKET_LEN 32
#define BROADCAST_ADDR 255
#define PAYLOAD_SIZE MAX_PACKET_LEN-7

class AodvPacket {
  public:
    AodvPacket();
    
    AodvPacket(byte src, byte dst, byte lHop, byte nHop, byte rseq, byte Cmd, byte hc, const byte* pld);
    
    void setPayload(byte* data, byte len);
    void readPayload(byte* data, byte len);

    byte source;
    byte destination;
    byte lastHop;
    byte nextHop;
    byte cmd;  
    byte hopCount;
    byte routeSeq;
    byte payload[PAYLOAD_SIZE];
    
    #ifdef DEBUG_BCNT
    void toSerial();
    #endif
    
    AodvPacket& operator=(const AodvPacket& other);
};

class SeenPackets {
  public:
    byte src;
    byte seq;
};

class BroadcastNetwork {
  public:
    BroadcastNetwork(byte MyAddr);
    
    boolean getPacket(AodvPacket& out);
    
    void sendPacket(AodvPacket& pkt);
    
    void init();
    
    byte getMyAddress();
    
  private:
    byte seq;
    SeenPackets packetTrack[BROADCAST_MEMORY];
    byte trackIndex;
    byte myaddr;
};

