#ifndef HASH_H
#define HASH_H

#include <stdint.h>

#define HASH_BUF_SIZE 32
void calculate_hash(uint8_t *buf, uint32_t filesize, uint8_t *hash_buf, uint8_t *hashsize);

#endif // HASH_H
