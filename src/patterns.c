#include "patterns.h"

#include <stdlib.h>

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);

}

static inline uint8_t* put_pixel(uint32_t pixel_grb, uint8_t *current_strip_out, bool current_strip_4color) {
    *current_strip_out++ = pixel_grb & 0xffu;
    *current_strip_out++ = (pixel_grb >> 8u) & 0xffu;
    *current_strip_out++ = (pixel_grb >> 16u) & 0xffu;
    if (current_strip_4color) {
        *current_strip_out++ = 0; // todo adjust?
    }
    return current_strip_out;
}

uint8_t* pattern_snakes(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t x = (i + (t >> 1)) % 64;
        if (x < 10)
            current_strip_out = put_pixel(urgb_u32(0xff, 0, 0), current_strip_out, current_strip_4color);
        else if (x >= 15 && x < 25)
            current_strip_out = put_pixel(urgb_u32(0, 0xff, 0), current_strip_out, current_strip_4color);
        else if (x >= 30 && x < 40)
            current_strip_out = put_pixel(urgb_u32(0, 0, 0xff), current_strip_out, current_strip_4color);
        else
            current_strip_out = put_pixel(0, current_strip_out, current_strip_4color);
    }
    return current_strip_out;
}

uint8_t* pattern_random(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    if (t % 8)
        return current_strip_out;
    for (uint32_t i = 0; i < len; ++i)
        current_strip_out = put_pixel(rand(), current_strip_out, current_strip_4color);
    return current_strip_out;
}

uint8_t* pattern_sparkle(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    if (t % 8)
        return current_strip_out;
    for (uint32_t i = 0; i < len; ++i)
        current_strip_out = put_pixel(rand() % 16 ? 0 : 0xffffffff, current_strip_out, current_strip_4color);
    return current_strip_out;
}

uint8_t* pattern_greys(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    uint8_t max = 100; // let's not draw too much current!
    t %= max;
    for (uint32_t i = 0; i < len; ++i) {
        current_strip_out = put_pixel(t * 0x10101, current_strip_out, current_strip_4color);
        if (++t >= max) t = 0;
    }
    return current_strip_out;
}

uint8_t* pattern_solid(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color, uint32_t color) {
    for (uint32_t i = 0; i < len; ++i) {
        current_strip_out = put_pixel(color, current_strip_out, current_strip_4color);
    }
    return current_strip_out;
}

uint8_t* pattern_solid_red(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    return pattern_solid(len, t, current_strip_out, current_strip_4color, urgb_u32(255,0,0));
}

uint8_t* pattern_solid_green(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    return pattern_solid(len, t, current_strip_out, current_strip_4color, urgb_u32(0,255,0));
}

uint8_t* pattern_solid_blue(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
    return pattern_solid(len, t, current_strip_out, current_strip_4color, urgb_u32(0,0,255));
}

int level = 8;

// void pattern_fade(uint32_t len, uint32_t t, uint8_t *current_strip_out, bool current_strip_4color) {
//     uint8_t shift = 4;

//     uint8_t max = 16; // let's not draw too much current!
//     max <<= shift;

//     uint8_t slow_t = t / 32;
//     slow_t = level;
//     slow_t %= max;

//     static uint8_t error = 0;
//     slow_t += error;
//     error = slow_t & ((1u << shift) - 1);
//     slow_t >>= shift;
//     slow_t *= 0x010101;

//     for (uint32_t i = 0; i < len; ++i) {
//         put_pixel(slow_t, current_strip_out, current_strip_4color);
//     }
// }

const pattern_t pattern_table[] = {
        {pattern_snakes,  "Snakes!"},
        {pattern_random,  "Random data!"},
        {pattern_sparkle, "Sparkles!"},
        {pattern_greys,   "Greys!"},
        {pattern_solid_red, "Solid red!"  },
        {pattern_solid_green, "Solid green!"  },
        {pattern_solid_blue, "Solid blue!"  },
        // {pattern_solid,  "Solid!"},
        // {pattern_fade, "Fade"},
};

const uint8_t pattern_count = sizeof(pattern_table) / sizeof(pattern_table[0]);