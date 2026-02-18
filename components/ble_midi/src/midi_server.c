#include "ble_midi.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include <string.h>

#define TAG "BLE_MIDI"
#define PROFILE_NUM 1
#define PROFILE_APP_ID 0
#define MIDI_SERVICE_UUID        0x03B8 // 16-bit short UUID for standard MIDI? No, custom 128-bit.
// MIDI Service: 03B80E5A-EDE8-4B33-A751-6CE34EC4C700
static uint8_t midi_service_uuid[16] = {
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7,
    0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03
};
// Characteristic: 7772E5DB-3868-4112-A1A9-F2669D106BF3
static uint8_t midi_char_uuid[16] = {
    0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1,
    0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77
};

static uint16_t midi_handle_table[4];
static esp_gatt_if_t gatts_if_global = 0;
static uint16_t conn_id_global = 0;
static bool connected = false;

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void ble_midi_init(void) {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
}

// Minimal GAP handler to start advertising
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            gatts_if_global = gatts_if;
            esp_ble_gap_set_device_name("ESP32_LaserHarp");

            esp_gatt_srvc_id_t service_id;
            service_id.is_primary = true;
            service_id.id.inst_id = 0x00;
            service_id.id.uuid.len = ESP_UUID_LEN_128;
            memcpy(service_id.id.uuid.uuid.uuid128, midi_service_uuid, 16);

            esp_ble_gatts_create_service(gatts_if, &service_id, 4);
            break;

        case ESP_GATTS_CREATE_EVT:
            esp_ble_gatts_start_service(param->create.service_handle);
            esp_bt_uuid_t char_uuid;
            char_uuid.len = ESP_UUID_LEN_128;
            memcpy(char_uuid.uuid.uuid128, midi_char_uuid, 16);

            esp_ble_gatts_add_char(param->create.service_handle, &char_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                NULL, NULL);
            break;

        case ESP_GATTS_ADD_CHAR_EVT: {
            midi_handle_table[0] = param->add_char.attr_handle;
            // Add config descriptor (CCCD) for notifications
            esp_bt_uuid_t descr_uuid;
            descr_uuid.len = ESP_UUID_LEN_16;
            descr_uuid.uuid.uuid16 = 0x2902;
            esp_ble_gatts_add_char_descr(param->add_char.service_handle, &descr_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            break;
        }

        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            midi_handle_table[1] = param->add_char_descr.attr_handle;
            ESP_LOGI(TAG, "Added CCCD handle: %d", midi_handle_table[1]);
            break;

        case ESP_GATTS_CONNECT_EVT:
            conn_id_global = param->connect.conn_id;
            connected = true;
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            connected = false;
            esp_ble_gap_start_advertising(NULL); // Restart adv
            break;
        default:
            break;
    }
}

void ble_midi_send_packet(uint8_t *data, size_t len) {
    if (connected && midi_handle_table[0] != 0) {
        esp_ble_gatts_send_indicate(gatts_if_global, conn_id_global, midi_handle_table[0], len, data, false);
    }
}

void ble_midi_send_note_on(uint8_t note, uint8_t velocity) {
    // BLE MIDI Packet: Header + Timestamp + Status + Note + Velocity
    // Header: 0x80 | (Time High 6 bit? No)
    // Simplified: 0x80 (Header), 0x80 (Timestamp High), 0x90 (Status Ch1 Note On), Note, Vel
    uint8_t packet[] = {0x80, 0x80, 0x90, note, velocity};
    ble_midi_send_packet(packet, sizeof(packet));
}

void ble_midi_send_note_off(uint8_t note) {
    uint8_t packet[] = {0x80, 0x80, 0x80, note, 0};
    ble_midi_send_packet(packet, sizeof(packet));
}
