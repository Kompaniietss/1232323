#pragma once

#include <stdint.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define FIRMWARE_PROGMEM PROGMEM
#define FIRMWARE_READ_BYTE(address) pgm_read_byte(address)
#else
#define FIRMWARE_PROGMEM
#define FIRMWARE_READ_BYTE(address) (*(address))
#endif
