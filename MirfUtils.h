#ifndef MIRF_UTILS_H
#define MIRF_UTILS_H

#define RF_PWR1 1
#define RF_PWR2 2
#define RF_PWR _BV(RF_PWR1) | _BV(RF_PWR2)


void setConfigBits(byte reg, byte fieldMask, byte value) {
  byte regValue;
  Mirf.readRegister(reg,&regValue,1);
  regValue &= ~(fieldMask);
  regValue |= value;
  Mirf.writeRegister(reg,&regValue,1);
}

void setRFPwrLevel(byte powerLevel) {
  // this only works because rf power are the first bits
  if (powerLevel > 3) {
    powerLevel = 3;
  }
  
  setConfigBits(RF_SETUP,RF_PWR,powerLevel);
}


void setRADDR2(uint8_t * adr) 
// Sets the receiving address
{
	Mirf.ceLow();
	Mirf.writeRegister(RX_ADDR_P2,adr,mirf_ADDR_LEN);
	Mirf.ceHi();
}

void setPipe2Settings(bool enabled, bool autoack) {
  setConfigBits(EN_AA, _BV(ENAA_P2), autoack ? _BV(ENAA_P2) : 0);
  setConfigBits(EN_RXADDR, _BV(ERX_P2), enabled ? _BV(ERX_P2) : 0);
}

void setMirfRetries(byte retries, byte delay) {
  setConfigBits(SETUP_RETR,0xff,(delay&0x0f)<<ARD | (retries&0x0f) << ARC);
}

#endif
