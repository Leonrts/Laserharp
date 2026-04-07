#include "input_capture.h"
#include "driver/gpio.h"
#include "esp_attr.h"

static input_callback_t s_callback = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    if (s_callback) {
        s_callback();
    }
}

void input_capture_init(int pin, input_callback_t cb) {
    s_callback = cb;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Trigger on rising edge (Light detected)
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    // Comparator (LM393) is Open Collector and needs a Pull-Up resistor.
    // If No Light (Dark): V+ (Sensor) < V- (Ref). Output Low (GND).
    // If Light: V+ > V-. Output Open (Pulled Up to Vcc).
    // We detect a Rising Edge when light hits the sensor.
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, gpio_isr_handler, (void*) pin);
}
