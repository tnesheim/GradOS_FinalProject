#ifndef __MS_PACMAN_SERVICE_H__
#define __MS_PACMAN_SERVICE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h" 
#include "ms_pacman.h"

//Nunchuck BLE constants       
#define BLE_MS_PACMAN_BASE_UUID {0xB3, 0x68, 0x06, 0x50, 0xF6, 0xB5, 0x11, 0xE4, 0xB9, 0xB2, 0x16, 0x97, 0xF9, 0x25, 0xEC, 0x7B}
#define BLE_MS_PACMAN_SERVICE_UUID        0x200C
#define BLE_MS_PACMAN_CHARACTERISTIC_UUID 0x200D
#define BLE_MS_PACMAN_CHARACTERISTIC_SIZE sizeof(uint8_t)*3

#define BLE_MS_PACMAN_OFFSET_ARROW    0
#define BLE_MS_PACMAN_OFFSET_BUTTON_A 1
#define BLE_MS_PACMAN_OFFSET_BUTTON_B 2

struct MsPacmanCtrls;
typedef struct MsPacmanCtrls MsPacmanCtrls;

typedef void (*ble_ms_pacman_ctrls_write_handler_t) (MsPacmanCtrls ms_pacman_ctrls);

/**@brief Ms. Pacman Service structure. This contains various status information for the service. */
typedef struct ble_ms_pacman_s
{
   uint16_t service_handle;
   ble_gatts_char_handles_t ms_pacman_char_handles;
   uint8_t                  uuid_type;
   uint16_t                 conn_handle;
   ble_ms_pacman_ctrls_write_handler_t ms_pacman_write_handler;
} ble_ms_pacman_t;

#endif