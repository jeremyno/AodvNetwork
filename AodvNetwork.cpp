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

AodvPacket::AodvPacket() : source(0), destination(0),lastHop(0),nextHop(0),routeSeq(0), cmd(0), hopCount(0){
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
void AodvPacket::toSerial() {
  Serial.print(String("Packet ") + source + " to " + destination + " through " + nextHop + " (from " + lastHop + ") CMD: " + cmd + ", hops " + hopCount + ", seq=" + routeSeq + ", data=");
  for(byte b = 0; b < PAYLOAD_SIZE; b++) {
    Serial.print(payload[b]);
    Serial.print(", ");
  }
  
  Serial.println("<EOM>");
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

BroadcastNetwork::BroadcastNetwork(byte MyAddr) : trackIndex(0), myaddr(MyAddr) {
  for(byte b = 0; b < BROADCAST_MEMORY; b++) {
    packetTrack[b].src = 0;
    packetTrack[b].seq = 0;
  }
}

void BroadcastNetwork::init() {
  Mirf.init();
  Mirf.setRADDR((byte *)"bcstr");
  Mirf.setTADDR((byte *)"bcstr");
  Mirf.payload = sizeof(AodvPacket);
  Mirf.config();
}
    
boolean BroadcastNetwork::getPacket(AodvPacket& out) {
  if (!Mirf.isSending() && Mirf.dataReady()) {
    AodvPacket readPkt;
    Mirf.getData((byte *) &readPkt);
    
    out = readPkt;
    
    boolean alreadySeen = false;
    for(byte i = 0; i < BROADCAST_MEMORY; i++) {
      if (packetTrack[i].src == readPkt.source && packetTrack[i].seq == readPkt.routeSeq) {
        alreadySeen = true;
      }
    }
    
    packetTrack[trackIndex].src = readPkt.source;
    packetTrack[trackIndex].seq = readPkt.routeSeq;
    trackIndex++;
    if (trackIndex > BROADCAST_MEMORY) {
      trackIndex = 0;
    }
    
    if (readPkt.destination == myaddr && !alreadySeen) {
      return true;
    }
            
    readPkt.hopCount++;
    readPkt.lastHop = myaddr;
    
    boolean forward = !alreadySeen && readPkt.hopCount <= MAX_HOPS && (readPkt.destination == BROADCAST_ADDR || readPkt.nextHop == myaddr );

    if (!forward) {
      return false;
    }
    
    delay(random(10));
    
    Mirf.send((byte*)&readPkt);
    
    while (Mirf.isSending()) {
    }
                    
    if (readPkt.destination == BROADCAST_ADDR) {
      return true;
    } else {
      return false;
    }
  }
  
  return false;
}
    
void BroadcastNetwork::sendPacket(AodvPacket& pkt) {
  seq++;
  pkt.routeSeq = seq;
  pkt.source=myaddr;
  pkt.lastHop=myaddr;
  
  Mirf.send((byte*)&pkt);
  while (Mirf.isSending()) {
  }
}
    
byte BroadcastNetwork::getMyAddress() {
  return myaddr;
}
