#include <stdio.h>
#include <stdlib.h>
#include "twi_master.h"
#include "nunchuck.h"
#include "nrf_delay.h"
#include "nunchuck_service.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf51_bitfields.h"
#include "boards.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "device_manager.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_debug_assert_handler.h"
#include "pstorage.h"
#include "app_trace.h"

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
   nunData->button_c   = ((dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_BUTTON_C) >> NUN_SHIFT_BUTTON_C) \
                           == NUN_BUTTON_ACTIVE_LOW ? NUN_BUTTON_NOT_PRESSED:NUN_BUTTON_PRESSED; 
   nunData->button_z   = (dataBuf[NUN_ACCEL_LSB_C_Z] & NUN_MASK_BUTTON_Z) \
                           == NUN_BUTTON_ACTIVE_LOW ? NUN_BUTTON_NOT_PRESSED:NUN_BUTTON_PRESSED;
   
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

/*Add the Nunchuck service and associated characteristic to the GATT server*/
uint32_t ble_nunchuck_init(ble_nunchuck_t *p_nunchuck)
{
   uint32_t   err_code;
   ble_uuid_t ble_uuid;

   // Initialize service structure
   p_nunchuck->conn_handle = BLE_CONN_HANDLE_INVALID;
   
   // Add Nunchuck service
   ble_uuid128_t base_uuid = {BLE_NUNCHUCK_BASE_UUID};
   err_code = sd_ble_uuid_vs_add(&base_uuid, &p_nunchuck->uuid_type);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }
   
   ble_uuid.type = p_nunchuck->uuid_type;
   ble_uuid.uuid = BLE_NUNCHUCK_SERVICE_UUID;
   
   err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_nunchuck->service_handle);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }

   err_code = nunchuck_char_add(p_nunchuck);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }
    
   return NRF_SUCCESS;
}

/*Add the Nunchuck characteristic*/
uint32_t nunchuck_char_add(ble_nunchuck_t *p_nunchuck)
{
   ble_gatts_char_md_t char_md;
   ble_gatts_attr_t    attr_char_value;
   ble_uuid_t          ble_uuid;
   ble_gatts_attr_md_t attr_md;

   memset(&char_md, 0, sizeof(char_md));
   
   char_md.char_props.read   = 1;
   char_md.char_props.write  = 0;
   char_md.char_props.notify = 1;
   char_md.p_char_user_desc  = NULL;
   char_md.p_char_pf         = NULL;
   char_md.p_user_desc_md    = NULL;
   char_md.p_cccd_md         = NULL;
   char_md.p_sccd_md         = NULL;
   
   ble_uuid.type = p_nunchuck->uuid_type;
   ble_uuid.uuid = BLE_NUNCHUCK_CHARACTERISTIC_UUID;
   
   memset(&attr_md, 0, sizeof(attr_md));
   
   BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
   BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
   attr_md.vloc       = BLE_GATTS_VLOC_STACK;
   attr_md.rd_auth    = 0;
   attr_md.wr_auth    = 0;
   attr_md.vlen       = 0;
   
   memset(&attr_char_value, 0, sizeof(attr_char_value));
   
   attr_char_value.p_uuid       = &ble_uuid;
   attr_char_value.p_attr_md    = &attr_md;
   attr_char_value.init_len     = BLE_NUNCHUCK_CHARACTERISTIC_SIZE;
   attr_char_value.init_offs    = 0;
   attr_char_value.max_len      = BLE_NUNCHUCK_CHARACTERISTIC_SIZE;
   attr_char_value.p_value      = NULL;
   
   return sd_ble_gatts_characteristic_add(p_nunchuck->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_nunchuck->nunchuck_char_handles);
}

/*Notifies central that data is ready*/
uint32_t ble_nunchuck_send(ble_nunchuck_t *p_nunchuck, NunchuckData *nunData)
{
   uint32_t err_code;
   
   //Send only if we are connected
   if(p_nunchuck->conn_handle != BLE_CONN_HANDLE_INVALID)
   {
      uint16_t len = BLE_NUNCHUCK_CHARACTERISTIC_SIZE;
      uint16_t data_len = len;
      uint8_t nunchuck_data[BLE_NUNCHUCK_CHARACTERISTIC_SIZE];
      ble_gatts_hvx_params_t hvx_params;
      
      //Fill in the Nunchuck data
      nunchuck_data[BLE_NUNCHUCK_OFFSET_JOY_X]    = nunData->joystick_x;
      nunchuck_data[BLE_NUNCHUCK_OFFSET_JOY_Y]    = nunData->joystick_y;
      nunchuck_data[BLE_NUNCHUCK_OFFSET_BUTTON_C] = nunData->button_c;
      nunchuck_data[BLE_NUNCHUCK_OFFSET_BUTTON_Z] = nunData->button_z;
      
      //Copy the accelerometer data
      memcpy(&nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_X], &(nunData->accel_x), sizeof(uint16_t));
      memcpy(&nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_Y], &(nunData->accel_y), sizeof(uint16_t));
      memcpy(&nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_Z], &(nunData->accel_z), sizeof(uint16_t));
      
      memset(&hvx_params, 0, sizeof(hvx_params));
      
      hvx_params.handle = p_nunchuck->nunchuck_char_handles.value_handle;
      hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
      hvx_params.offset = 0;
      hvx_params.p_len  = &data_len;
      hvx_params.p_data = nunchuck_data;
      
      err_code = sd_ble_gatts_hvx(p_nunchuck->conn_handle, &hvx_params);
      if ((err_code == NRF_SUCCESS) && (data_len != len))
      {
         err_code = NRF_ERROR_DATA_SIZE;
      }
   }
   else
   {
      err_code = NRF_ERROR_INVALID_STATE;
   }
   
   return err_code;
}