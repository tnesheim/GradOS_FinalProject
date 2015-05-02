#ifndef __NUNCHUCK_H__
#define __NUNCHUCK_H__

#include <stdio.h>
#include <stdlib.h>
#include "twi_master.h"

//Nunchuck constants
#define NUN_TWI_ADDRESS 0xA4
#define NUN_INIT_LENGTH 2
#define NUN_DATA_LENGTH 6
#define NUN_DATA_REQUEST_LENGTH 1
#define NUN_DATA_REQUEST_VALUE 0x00

//Nunchuck register initialization values
#define NUN_INIT_REG_0_BYTE_0 0xF0
#define NUN_INIT_REG_0_BYTE_1 0x55
#define NUN_INIT_REG_1_BYTE_0 0xFB
#define NUN_INIT_REG_1_BYTE_1 0x00
#define NUN_DATA_REQUEST_VALUE 0x00

//Nunchuck last byte masks
#define NUN_MASK_ACCEL_Z_LSB 0xC0
#define NUN_MASK_ACCEL_Y_LSB 0x30
#define NUN_MASK_ACCEL_X_LSB 0x0C
#define NUN_MASK_BUTTON_C    0x02
#define NUN_MASK_BUTTON_Z    0x01

//Nunchuck last byte shift amounts
#define NUN_SHIFT_ACCEL_MSB   2
#define NUN_SHIFT_ACCEL_Z_LSB 6
#define NUN_SHIFT_ACCEL_Y_LSB 4
#define NUN_SHIFT_ACCEL_X_LSB 2
#define NUN_SHIFT_BUTTON_C    1

typedef enum
{
   NUN_JOY_X_OFFSET = 0,
   NUN_JOY_Y_OFFSET,
   NUN_ACCEL_X_OFFSET,
   NUN_ACCEL_Y_OFFSET,
   NUN_ACCEL_Z_OFFSET,
   NUN_ACCEL_LSB_C_Z,
} NunchuckDataOffsets;

typedef struct
{
   uint8_t joystick_x;
   uint8_t joystick_y;
   uint16_t accel_x;
   uint16_t accel_y;
   uint16_t accel_z;
   uint8_t button_c;
   uint8_t button_z;
} NunchuckData;

/*Initializes the Wii Nunchuck
 *return = true if successfully initialized Nunchuck, false otherwise*/
bool initNunchuck(void);

/*Reads the data from the Nunchuck*/
bool readNunchuckData(NunchuckData *nunData);

#endif 