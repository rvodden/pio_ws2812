#ifndef PATTERN_H
#define PATTERN_H


#include <stdint.h>
#include <stdbool.h>

typedef uint8_t* (*pattern)(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color);

typedef struct {
    pattern pat;
    const char *name;
} pattern_t;

extern const pattern_t pattern_table[];
extern const uint8_t pattern_count;

#endif // PATTERN_H