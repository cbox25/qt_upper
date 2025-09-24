#ifndef CRC_H
#define CRC_H

#include <unistd.h>
#include <stdint.h>

#define CRC_POLY_16		0xA001
#define CRC_POLY_32		0xEDB88320ul
#define CRC_START_8		0x00
#define CRC_START_16	0x0000
#define CRC_START_32	0xFFFFFFFFul

/* Determine whether the data packet is complete */
int check_crc8(uint8_t *buffer, uint32_t len, uint8_t crc);
int check_crc16(uint8_t *buffer, uint32_t len, uint16_t crc);
int check_crc32(uint8_t *buffer, uint32_t len, uint32_t crc);

/* Generate the CRC check code */
uint8_t  calc_crc8(const uint8_t *buf, uint32_t bytes);
uint16_t calc_crc16(const uint8_t *buf, uint32_t bytes);
uint32_t calc_crc32(const uint8_t *buf, uint32_t bytes);

int CheckSum(uint8_t *data, uint32_t len, uint32_t chkSum);

#endif

