#ifndef __NUNCHUCK_SERVICE_H__
#define __NUNCHUCK_SERVICE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h" 

//Nunchuck BLE constants       
#define BLE_NUNCHUCK_BASE_UUID {0xB4, 0x47, 0x9E, 0xFA, 0xF6, 0x9C, 0x11, 0xE4, 0xB9, 0xB2, 0x16, 0x97, 0xF9, 0x25, 0xEC, 0x7B}
#define BLE_NUNCHUCK_SERVICE_UUID        0x200A
#define BLE_NUNCHUCK_CHARACTERISTIC_UUID 0x200B
#define BLE_NUNCHUCK_CHARACTERISTIC_SIZE sizeof(uint8_t)*4 + sizeof(uint16_t)*3

#define BLE_NUNCHUCK_OFFSET_JOY_X    0
#define BLE_NUNCHUCK_OFFSET_JOY_Y    1
#define BLE_NUNCHUCK_OFFSET_ACCEL_X  2
#define BLE_NUNCHUCK_OFFSET_ACCEL_Y  4
#define BLE_NUNCHUCK_OFFSET_ACCEL_Z  6
#define BLE_NUNCHUCK_OFFSET_BUTTON_C 8
#define BLE_NUNCHUCK_OFFSET_BUTTON_Z 9

/**@brief Nunchuck Service structure. This contains various status information for the service. */
typedef struct ble_nunchuck_s
{
   uint16_t service_handle;
   ble_gatts_char_handles_t nunchuck_char_handles;
   uint8_t                  uuid_type;
   uint16_t                 conn_handle;
   
} ble_nunchuck_t;

#endif