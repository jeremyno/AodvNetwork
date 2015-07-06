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
#include "AodvNetwork.h"
#include <SerialDebugOn.h>

AodvPacket::AodvPacket() : source(0), destination(0),lastHop(0),nextHop(0),routeSeq(0), cmd(0), hopCount(0){
}

AodvPacket::AodvPacket(byte dst, byte Cmd) : source(0), destination(dst),lastHop(0),nextHop(0),routeSeq(0), cmd(Cmd), hopCount(0){
}
AodvPacket::AodvPacket(byte src, byte dst, byte lHop, byte nHop, byte rseq, byte Cmd, byte hc, const byte* pld) {
  source = src;
  destination = dst;
  nextHop = nHop;
  lastHop = lHop;
  routeSeq = rseq;
  cmd = Cmd;
  hopCount = hc;
  if (pld != NULL) {
    for(byte b = 0; b < PAYLOAD_SIZE; b++) {
      payload[b] = pld[b];
    }
  }
}

void AodvPacket::setPayload(byte* data, byte len) {
  for(byte b = 0; b < len; b++) {
    payload[b] = data[b];
  }
}
void AodvPacket::readPayload(byte* data, byte len) {
  for(byte b = 0; b < len; b++) {
    data[b] = payload[b];
  }
}

#ifdef DEBUG_BCNT
inline void printPacketHeader(AodvPacket& pkt, String cmdName) {
  Serial.print(String("Packet [source=") + pkt.source + ",destination=" + pkt.destination + ",nextHop=" + pkt.nextHop + ",lastHop=" + pkt.lastHop + ",command=" + cmdName + "(" + pkt.cmd + "),hops=" + pkt.hopCount + ",seq=" + pkt.routeSeq + "] data=");
}

inline void printRawData(byte* payload) {
  for(byte b = 0; b < PAYLOAD_SIZE; b++) {
    Serial.print(payload[b]);
    Serial.print(", ");
  }
}

inline void printRouteRequest(AodvPacket& pkt) {
  RouteResponsePayload rrpld;
  pkt.readPayload(rrpld);
  
  Serial.print("RouteResponse <Count=");
  Serial.print(rrpld.count);
  Serial.print("> Hops: [");
  for(byte b = 0; b < rrpld.count; b++) {
    Serial.print(rrpld.hops[b]);
    Serial.print(",");
  }
  Serial.print("]");
}

void AodvPacket::toSerial() {
  switch (cmd) {
    case 0:
      printPacketHeader(*this,"DATA");
      printRawData(payload);
      break;
    case 1:
      printPacketHeader(*this,"ARP");
      printRawData(payload);
      break;
    case 2:
      printPacketHeader(*this,"DESCRIBE");
      printRawData(payload);
      break;
    case 3:
      printPacketHeader(*this,"ROUTE REQUEST");
      printRouteRequest(*this);
      break;
    case 4:
      printPacketHeader(*this,"ROUTE RESPONSE");
      printRouteRequest(*this);
      break;
    default:
      printPacketHeader(*this,String("")+cmd);
      printRawData(payload);
      break;
  }
  
  Serial.println("<EOM>");
}

void BroadcastNetwork::dumpRoutes() {
  for(byte i = 0; i < ROUTE_MEMORY; i++) {
    DistanceVector& v = routes[i];
    
    debugln(String("  Route for ") + v.destination + " via " + v.nextHop + " over " + v.getHops() + " hops with time: " + v.getTime());
  }
}
#endif
    
AodvPacket& AodvPacket::operator=(const AodvPacket& other) {
  if (this != &other) {
    source = other.source;
    destination = other.destination;
    lastHop = other.lastHop;
    nextHop = other.nextHop;
    cmd = other.cmd;
    hopCount = other.hopCount;
    routeSeq = other.routeSeq;
    for(byte b = 0; b < PAYLOAD_SIZE; b++) {
      payload[b] = other.payload[b];
    }
  }
      
  return *this;
}

BroadcastNetwork::BroadcastNetwork(byte MyAddr) : trackIndex(0), myaddr(MyAddr), lastTick(0) {
  for(byte b = 0; b < BROADCAST_MEMORY; b++) {
    packetTrack[b].src = 0;
    packetTrack[b].seq = 0;
  }
  
  for(byte b = 0; b < ROUTE_MEMORY; b++) {
    routes[b].destination = 0;
    routes[b].nextHop = 0;
    routes[b].seq = 0;
    routes[b].hopCountAndTime = 0;
  }
}

void BroadcastNetwork::init() {
  Mirf.init();
  setAddr(myaddr);
  setCRC2bytes(true);
  Mirf.setRADDR((byte*)"bcast");
  Mirf.setTADDR((byte*)"bcast");
  // don't auto ack 
  // setConfigBits(EN_AA, _BV(ENAA_P1), 0);
  Mirf.payload = sizeof(AodvPacket);
  Mirf.config();
}

void BroadcastNetwork::setAddr(byte b) {
  myaddr = b;
}
    
boolean BroadcastNetwork::getPacket(AodvPacket& out) {
  if (millis() - lastTick > TIMEOUT_TICK) {
    for(byte i = 0; i < ROUTE_MEMORY; i++) {
      routes[i].decrementTime();
    }
    lastTick = millis();    
  }

  if (!Mirf.isSending() && Mirf.dataReady()) {
    AodvPacket readPkt;
    Mirf.getData((byte *) &readPkt);
    
    /*
    if ((myaddr == 1 && readPkt.lastHop == 3) || (myaddr == 3 && readPkt.lastHop == 1)) {
      return false;
    }
    */
    
    out = readPkt;
    DEBUGCMD(readPkt.toSerial();)
    
    // this is a rreq, we actually don't track "already seen" because
    // we want any multiroutes to be forwarded to find the BEST routes
    if (readPkt.cmd == AODV_CMD_RREQ) {
      debugln(String("  Got a Route Request Packet for ") + readPkt.destination);
      // always add source as a route
      RouteResponsePayload rrpld;
      readPkt.readPayload(rrpld);
            
      // if we're already in any hops, just drop out
      for(byte b = 0; b < rrpld.count; b++) {
        if (rrpld.hops[b] == myaddr) {
          debugln("  We are already in this route. Ignoring");
          return false;
        }
      }
      
      updateRoute(readPkt.source, readPkt.hopCount, readPkt.routeSeq, readPkt.lastHop);

      rrpld.hops[rrpld.count] = myaddr;
      rrpld.count++;
      
      if (readPkt.destination == myaddr) {
        debugln("  This route request is for me!");
        readPkt.destination = readPkt.source;
        readPkt.nextHop = tryRoute(readPkt.destination);;
        readPkt.source = myaddr;
        readPkt.lastHop = myaddr;
        readPkt.routeSeq = ++seq;
        readPkt.cmd = AODV_CMD_RRESP;
        readPkt.hopCount = 0;
        readPkt.setPayload(rrpld);
        
        transmit(readPkt);
        debugln("  Returned a route response packet!");        
      } else {
        // update with our new hops
        readPkt.setPayload(rrpld);
        readPkt.nextHop = BROADCAST_ADDR;
        readPkt.lastHop = myaddr;
        readPkt.hopCount++;

        transmit(readPkt);
      }
      
      return false;
    }
    
    if ( readPkt.cmd == AODV_CMD_RRESP) {
      RouteResponsePayload rrpld;
      readPkt.readPayload(rrpld);
      debug("  Got a route response packet! Hops: ");
      debugln(rrpld.count);
      
      updateRoute(readPkt.source, readPkt.hopCount, readPkt.routeSeq, readPkt.lastHop);
      
      if (readPkt.destination != myaddr) {
        readPkt.lastHop=myaddr;
        readPkt.nextHop = tryRoute(readPkt.destination);
        readPkt.hopCount++;
        
        transmit(readPkt);
      }
      
      return false;
    }
    
    boolean alreadySeen = false;
    for(byte i = 0; i < BROADCAST_MEMORY; i++) {
      if (packetTrack[i].src == readPkt.source && packetTrack[i].seq == readPkt.routeSeq) {
        alreadySeen = true;
      }
    }
    
    packetTrack[trackIndex].src = readPkt.source;
    packetTrack[trackIndex].seq = readPkt.routeSeq;
    trackIndex++;
    if (trackIndex >= BROADCAST_MEMORY) {
      trackIndex = 0;
    }

    // we don't add neighbors we've heard directly because it causes issues    
    // if they have higher transmit power than us.
    
    if (readPkt.destination == myaddr && !alreadySeen) {
      return true;
    }
    
    readPkt.hopCount++;
    readPkt.lastHop = myaddr;
    
    boolean forward = !alreadySeen && (readPkt.destination == BROADCAST_ADDR || readPkt.nextHop == myaddr );
    
    debugln(String("  Should forward? ") + forward);
    
    if (!forward) {
      debugln("  should not forward. giving up.");
      return false;
    }
    
    readPkt.nextHop = tryRoute(readPkt.destination);
    
    debugln("  Forwarded...");
    transmit(readPkt);   
    
    if (readPkt.destination == BROADCAST_ADDR) {
      return true;
    } else {
      return false;
    }
  }
  
  return false;
}

byte BroadcastNetwork::tryRoute(byte destination) {
  debugln(String("  Trying to find route to ")+destination);
  if (destination == BROADCAST_ADDR) {
    return BROADCAST_ADDR;
  }
  
  #ifdef DEBUG_BCNT
  dumpRoutes();
  #endif
  
  for(byte i = 0; i < ROUTE_MEMORY; i++) {
    DistanceVector& v = routes[i];
    
    if (v.destination == destination && v.getTime() > 0) {
      // valid route
      debugln(String("  Discovered route to ") + destination + " via " + v.nextHop + " over " + v.getHops() + " hops");
      
      return v.nextHop;
    }    
  }
  
  // make a request for a route, but we can just broadcast route the first
  // packet in the series. Hopefully, the route request returns and we 
  // are able to route it by the next time
  AodvPacket rreq(destination,AODV_CMD_RREQ);
  seq++;
  rreq.routeSeq = seq;
  rreq.lastHop = myaddr;
  rreq.nextHop = BROADCAST_ADDR;
  rreq.source = myaddr;
  RouteResponsePayload pld;
  pld.count = 0;
  rreq.setPayload(pld);

  transmit(rreq);

  debugln("  No route found. Sent RREQ and broadcast routing");  
  return BROADCAST_ADDR;  
}
    
void BroadcastNetwork::sendPacket(AodvPacket& pkt) {
  debugln("Sending a packet");
  seq++;
  pkt.routeSeq = seq;
  pkt.source=myaddr;
  pkt.lastHop=myaddr;
  pkt.nextHop = tryRoute(pkt.destination);
  
  transmit(pkt);  
}

void BroadcastNetwork::transmit(AodvPacket& pkt) {
  // don't transmit for ourselves
  if (pkt.destination == myaddr || pkt.nextHop == myaddr) {
    return;
  }
  
  // don't transmit over max_hops
  if (pkt.hopCount > MAX_HOPS) {
    return;
  }
  
  for(byte b = 0; b < TRANSMIT_TIMES; b++) {
    delay(random(10));
    Mirf.send((byte*)&pkt);
    
    while (Mirf.isSending()) {
    }
  }
}
    
byte BroadcastNetwork::getMyAddress() {
  return myaddr;
}

void BroadcastNetwork::updateRoute(byte destination, byte hops, byte seq, byte nextHop) {
  // routing to ourselves is easy... we don't.
  if (destination == myaddr) {
    return; 
  }
  debugln(String("  call: update route to <dest=")+destination+",hops="+hops+",seq="+seq+",nextHop="+nextHop+">");
  
  byte lowestIndex = 0;
  byte lowestTime = 16;
  boolean found = false;
  
  for(byte i = 0; i < ROUTE_MEMORY; i++) {
    DistanceVector& v = routes[i];
    
    if (lowestTime > v.getTime()) {
      lowestIndex = i;
      lowestTime = v.getHops();
    }
    
    if (v.destination == destination) { 
      debugln(String("  Comparing update for ") + v.destination + " via " + v.nextHop + " over " + v.getHops() + " hops with time: " + v.getTime());
      found = true;
      // if responding to same route request, 

      if (v.seq == seq) {
        if (v.getHops() > hops) {
          v.setTime(15);
          v.setHops(hops);
          v.nextHop = nextHop;
          debugln("    updated <1>.");
        }
        // old seq or old seq with overflow, or if expired
      } else if(v.seq < seq  || (v.seq - seq > 240) || v.getTime() == 0) {
        v.setTime(15);
        v.setHops(hops);
        v.seq = seq;
        v.nextHop = nextHop;
        debugln("    updated <2>.");
      }
    }
  }
  
  if (!found) {
    DistanceVector& v = routes[lowestIndex];

    v.destination = destination;
    v.nextHop = nextHop;
    v.seq = seq;
    v.setTime(15);
    v.setHops(hops);
    debugln("  updated (Not Found).");
  }
}

 void BroadcastNetwork::setConfigBits(byte reg, byte fieldMask, byte value) {
  byte regValue;
  Mirf.readRegister(reg,&regValue,1);
  regValue &= ~(fieldMask);
  regValue |= value;
  Mirf.writeRegister(reg,&regValue,1);
}

 void BroadcastNetwork::setRFPwrLevel(byte powerLevel) {
  // this only works because rf power are the first bits
  if (powerLevel > 3) {
    powerLevel = 3;
  }
  
  setConfigBits(RF_SETUP,RF_PWR,powerLevel);
}

// void BroadcastNetwork::setMirfRetries(byte retries, byte delay) {
//  setConfigBits(SETUP_RETR,0xff,(delay&0x0f)<<ARD | (retries&0x0f) << ARC);
//}
