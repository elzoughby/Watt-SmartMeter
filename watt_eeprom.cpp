#include <EEPROM.h>     //Arduino Library

void eepromBegin(unsigned int size) {
  EEPROM.begin(size);
}


void eepromStore(unsigned int address, unsigned int val) {

  EEPROM.write(address, 255);
  address++;
  if(val < 255)
    EEPROM.write(address, val);
  else {
    int n = val / 255;
    for(int i=0; i<n; i++) {
      EEPROM.write(address, 255);
      address++;
    }
    if(val%255 !=0) {
      EEPROM.write(address, (val-(n*255)));
      address++;
    }
  }
  EEPROM.write(address, 0);
  EEPROM.commit();
}


unsigned int eepromRead(unsigned int address) {

  unsigned int result = 0;

  if( EEPROM.read(address) == 255 ) {
    address++;
    while(EEPROM.read(address) != 0) {
      result += EEPROM.read(address);
      address++;
    }
    address++;
    return result;
  }
  else
    return 0;
}
