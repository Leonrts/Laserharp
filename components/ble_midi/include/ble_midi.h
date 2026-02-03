#ifndef BLE_MIDI_H
#define BLE_MIDI_H

#include <stdint.h>

void ble_midi_init(void);
void ble_midi_send_note_on(uint8_t note, uint8_t velocity);
void ble_midi_send_note_off(uint8_t note);

#endif
