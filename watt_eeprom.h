#define CUMULATIVE_ADDRESS 0
#define YEAR_CUMULATIVE_ADDRESS 20
#define MONTH_CUMULATIVE_ADDRESS 40
#define DAY_CUMULATIVE_ADDRESS 60
#define HOUR_CUMULATIVE_ADDRESS 80
#define CURR_HOUR_ADDRESS 100
#define CURR_DAY_ADDRESS 120
#define CURR_MONTH_ADDRESS 140
#define CURR_YEAR_ADDRESS 160

void eepromBegin(unsigned int size);

void eepromStore(unsigned int address, unsigned int val);

unsigned int eepromRead(unsigned int address);

//TODO a problem with floating points values