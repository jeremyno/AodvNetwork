#include "AodvUDPProto.h"

void UdpPayload::setPayload(byte* data, byte len) {
  for(byte b = 0; b < len; b++) {
    payload[b] = data[b];
  }
}
void UdpPayload::readPayload(byte* data, byte len) {
  for(byte b = 0; b < len; b++) {
    data[b] = payload[b];
  }
}
