#define CUMULATIVE_ADDRESS 0
#define YEAR_CUMULATIVE_ADDRESS 20
#define MONTH_CUMULATIVE_ADDRESS 30
#define DAY_CUMULATIVE_ADDRESS 40
#define HOUR_CUMULATIVE_ADDRESS 50
#define CURR_HOUR_ADDRESS 60
#define CURR_DAY_ADDRESS 65
#define CURR_MONTH_ADDRESS 70
#define CURR_YEAR_ADDRESS 75

void eepromBegin(unsigned int size);

void eepromStore(unsigned int address, unsigned int val);

unsigned int eepromRead(unsigned int address);
