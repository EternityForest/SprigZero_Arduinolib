#ifndef SPRIGZERO_H
#define SPRIGZERO_H
void startTimer2RTCExt( void );
uint16_t readVCC();
uint16_t doPowerManagement(uint8_t seconds=0);
void setUnixTime(uint32_t t);
uint32_t getUnixTime();
uint16_t readmV(uint8_t);

#define LDO_CONTROL 2
#define USER_SWITCH_PIN 5
#define REF_PIN A4
#endif
