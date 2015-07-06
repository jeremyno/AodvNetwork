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
#ifndef AODV_NETWORK_H
#define AODV_NETWORK_H

#include "arduino.h"
#include <Mirf.h>
#include "MirfUtils.h"

//#define DEBUG_BCNT
#ifdef DEBUG_BCNT
#define DEBUG_ON
#define DEBUGCMD(x) x
#else
#define DEBUGCMD(x)
#endif

#define MAX_ROUTES 10
#define MAX_HOPS 5
#define BROADCAST_MEMORY 10
#define ROUTE_MEMORY 10
#define MAX_PACKET_LEN 32
#define BROADCAST_ADDR 255
#define PAYLOAD_SIZE MAX_PACKET_LEN-7

#define AODV_CMD_DATA 0
#define AODV_CMD_ARP 1
#define AODV_CMD_DESCRIBE 2
#define AODV_CMD_RREQ 3
#define AODV_CMD_RRESP 4
#define AODV_CMD_PROTO_UDP 5
#define AODV_CMD_PROTO_TCP 6

// 8000 - 2 min
// 20000 - 5 min
// 40000 - 10 min
#define TIMEOUT_TICK 40000

#define RF_PWR1 1
#define RF_PWR2 2
#define RF_PWR _BV(RF_PWR1) | _BV(RF_PWR2)
#define TRANSMIT_TIMES 3

class AodvPacket {
  public:
    AodvPacket();
    // easy one
    AodvPacket(byte dest, byte Cmd);
    AodvPacket(byte src, byte dst, byte lHop, byte nHop, byte rseq, byte Cmd, byte hc, const byte* pld);
    
    void setPayload(byte* data, byte len);
    void readPayload(byte* data, byte len);
    
    template<class T> void setPayload(T& ref) {
      setPayload((byte*)&ref,sizeof(T));
    }
    
    template<class T> void readPayload(T& ref) {
      readPayload((byte*)&ref,sizeof(T));
    }


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

class RouteResponsePayload {
  public:
    byte count;
    byte invalidateSeq;
    byte hops[PAYLOAD_SIZE-2];
};

class DistanceVector {
  public:
    byte destination;
    byte nextHop;
    byte seq;
    byte hopCountAndTime;
    
    byte getHops() {
      return (hopCountAndTime & 0xF0) >> 4;
    }
        
    byte getTime() {
      return (hopCountAndTime & 0x0F);
    }
    
    void setHops(byte hops) {
      hopCountAndTime = (hopCountAndTime & 0x0F) | (hops << 4);
    }
    
    void setTime(byte time) {
      hopCountAndTime = (hopCountAndTime & 0xF0) | (time & 0x0F);
    }
    
    void decrementTime() {
      if ((hopCountAndTime & 0x0F) > 0) {
        hopCountAndTime--;
      }
    }
};

class BroadcastNetwork {
  public:
    BroadcastNetwork(byte MyAddr);
    
    boolean getPacket(AodvPacket& out);
    
    
    void sendPacket(AodvPacket& pkt);
    
    void init();
    
    byte getMyAddress();

    void setAddr(byte addr);
    void setRFPwrLevel(byte powerLevel);
  private:
    void updateRoute(byte destination, byte hops, byte seq, byte nextHop);
    byte tryRoute(byte destination);
    void transmit(AodvPacket& pkt);
    byte myaddr;
    byte seq;
    long lastTick;
    SeenPackets packetTrack[BROADCAST_MEMORY];
    byte trackIndex;
    DistanceVector routes[ROUTE_MEMORY];

    #ifdef DEBUG_BCNT
    void dumpRoutes();
    #endif

    void setConfigBits(byte reg, byte fieldMask, byte value);
    //void setMirfRetries(byte retries, byte delay);
};
#endif