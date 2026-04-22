#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Cli_Debug(const char *format, ...);
void Cli_Debug_Hex(char *ptr, uint8_t len, ...);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
