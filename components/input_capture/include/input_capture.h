#ifndef INPUT_CAPTURE_H
#define INPUT_CAPTURE_H

typedef void (*input_callback_t)(void);

// Initialize the GPIO interrupt
void input_capture_init(int pin, input_callback_t cb);

#endif
