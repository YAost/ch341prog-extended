#ifndef __CH341A_EXT_H__
#define __CH341A_EXT_H__

#define     MAXIMUM_SECTOR_SIZE     0x10000


#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include "ch341a.h"

int32_t SpiReadSegment(uint8_t *buffer, uint32_t from, uint32_t to);                    // read data from any (logical) segment (via address)
int32_t SpiWriteSegment(uint8_t *buffer, uint32_t from, uint32_t to, uint32_t sectSize);// write data to any (logical) segment (no need to erase first), set minimal size of physical sector
int32_t SpiWriteSector(uint8_t *buffer, uint32_t from, uint32_t to);                    // write data in physical sectors (should be erased first)
int32_t SpiEraseSector(uint32_t addr, uint32_t sectSize);      				// erase physical sector (set size of it)
int32_t SetDeviceByBus(uint8_t bus, uint8_t dev);                          		// device inititalisation with ability to use multiple devices
/*              processing                  */                                          // send data to SPI bus via opcode, address and buffer
/*              processing                  */                                          // verification function. Check if consisted data == desired
/*              processing                  */                                          // show available USB devices via bus or SerialNumber (descriptor)




#endif // __CH341A_EXT_H__
