#ifndef __MS_PACMAN_SERVICE_H__
#define __MS_PACMAN_SERVICE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ms_pacman.h"

//Nunchuck BLE constants       
#define BLE_MS_PACMAN_BASE_UUID {0xB3, 0x68, 0x06, 0x50, 0xF6, 0xB5, 0x11, 0xE4, 0xB9, 0xB2, 0x16, 0x97, 0xF9, 0x25, 0xEC, 0x7B}
#define BLE_MS_PACMAN_SERVICE_UUID        0x200C
#define BLE_MS_PACMAN_CHARACTERISTIC_UUID 0x200D
#define BLE_MS_PACMAN_CHARACTERISTIC_SIZE sizeof(uint8_t)*3

#define BLE_MS_PACMAN_OFFSET_ARROW    0
#define BLE_MS_PACMAN_OFFSET_BUTTON_A 1
#define BLE_MS_PACMAN_OFFSET_BUTTON_B 2

#endif
