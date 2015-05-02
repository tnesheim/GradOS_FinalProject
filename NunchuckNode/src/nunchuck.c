#include <stdio.h>
#include <stdlib.h>
#include "twi_master.h"
#include "nunchuck.h"
#include "nrf_delay.h"

void nrf_delay_ms(uint32_t delay)
{
   int i;
   
   for(i = 0; i < delay; i++)
   {
      nrf_delay_us(1000);
   }
}

/*Initializes the Wii Nunchuck
 *return = true if successfully initialized Nunchuck, false otherwise*/
bool initNunchuck(void)
{
   uint8_t initBuf[NUN_INIT_LENGTH];
   
   //Initialize the TWI master bus
   if(!twi_master_init())
   {
      return false;
   }
   
   //Initialize the first Nunchuck register
   initBuf[0] = NUN_INIT_REG_0_BYTE_0;
   initBuf[1] = NUN_INIT_REG_0_BYTE_1;
   
   //Write the first register
   if(!twi_master_transfer(NUN_TWI_ADDRESS, initBuf, NUN_INIT_LENGTH, TWI_ISSUE_STOP))
   {
      return false;
   }
   
   //Delay between writes
   nrf_delay_ms(1);
   
   //Initialize the second Nunchuck register
   initBuf[0] = NUN_INIT_REG_1_BYTE_0;
   initBuf[1] = NUN_INIT_REG_1_BYTE_1;
   
   //Write the first register
   if(!twi_master_transfer(NUN_TWI_ADDRESS, initBuf, NUN_INIT_LENGTH, TWI_ISSUE_STOP))
   {
      return false;
   }
   
   return true;
}

/*Reads the data from the Nunchuck*/
bool readNunchuckData(NunchuckData *nunData)
{
   uint8_t dataBuf[NUN_DATA_LENGTH];
   
   //Delay between data reads/writes of the Nunchuck
   nrf_delay_ms(1);
   
   //Send a zero to request data
   dataBuf[0] = NUN_DATA_REQUEST_VALUE;
   
   if(!twi_master_transfer(NUN_TWI_ADDRESS, dataBuf, NUN_DATA_REQUEST_LENGTH, TWI_ISSUE_STOP))
   {
      return false;
   }
   
   //Delay between data reads/writes of the Nunchuck
   nrf_delay_ms(1);
   
   //Read the 6 bytes of Nunchuck data 
   if(!twi_master_transfer(NUN_TWI_ADDRESS | TWI_READ_BIT, dataBuf, NUN_DATA_LENGTH, TWI_ISSUE_STOP))
   {
      return false;
   }
   
   //Store the received data in the NunchuckData struct
   nunData->joystick_x = dataBuf[NUN_JOY_X_OFFSET];
   nunData->joystick_y = dataBuf[NUN_JOY_Y_OFFSET];
   nunData->accel_x    = dataBuf[NUN_ACCEL_X_OFFSET];
   nunData->accel_y    = dataBuf[NUN_ACCEL_Y_OFFSET];
   nunData->accel_z    = dataBuf[NUN_ACCEL_Z_OFFSET];
   
   //Do fancy bit ands and shifts to get the button values stored in the last byte
   nunData->button_c   = (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_BUTTON_C) >> NUN_SHIFT_BUTTON_C; 
   nunData->button_z   = (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_BUTTON_Z);
   
   //Do some more fancy things to place the 10bit accelerometer data in its proper place
   //Shift the MSB of the 10bits up to make room for the bottom 2 bits from the last byte
   nunData->accel_x = nunData->accel_x << NUN_SHIFT_ACCEL_MSB;
   nunData->accel_y = nunData->accel_y << NUN_SHIFT_ACCEL_MSB;
   nunData->accel_z = nunData->accel_z << NUN_SHIFT_ACCEL_MSB;
   
   //Mask out the corresponding 2 lsb bits, shift them to their proper places, and OR them into the data
   nunData->accel_x |= (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_ACCEL_X_LSB) >> NUN_SHIFT_ACCEL_X_LSB;
   nunData->accel_y |= (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_ACCEL_Y_LSB) >> NUN_SHIFT_ACCEL_Y_LSB;
   nunData->accel_z |= (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_ACCEL_Z_LSB) >> NUN_SHIFT_ACCEL_Z_LSB;
   
   return true;
}