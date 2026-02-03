#ifndef LASER_ENGINE_H
#define LASER_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t i;
} ilda_point_t;

void laser_engine_init(void);
void laser_engine_start(void);
void laser_set_harp_strings(int num_strings, uint16_t *x_positions);

// Called by ISR when sensor triggers
void laser_engine_register_hit(void);

#endif
