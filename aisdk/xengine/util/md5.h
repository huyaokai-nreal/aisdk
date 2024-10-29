#ifndef _MD5_H_
#define _MD5_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
void md5str(uint8_t *digest, char *str);
#ifdef __cplusplus
}
#endif

#endif