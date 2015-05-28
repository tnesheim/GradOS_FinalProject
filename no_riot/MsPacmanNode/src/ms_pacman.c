#include <stdio.h>
#include <stdlib.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "ms_pacman.h"
#include "ms_pacman_service.h"

/*Initialize the Ms. Pacman GPIO lines to be outputs and 0xFF*/
void initMsPacmanGPIO(void)
{
   //Set pins as output
   nrf_gpio_range_cfg_output(GPIO_CTRLS_START, GPIO_CTRLS_END);
   
   //Turn all the relays off
   nrf_gpio_port_set(GPIO_RELAY_PORT, 0xFF);
}

/*Sets the GPIO pins based on the input data*/
void setMsPacmanControls(MsPacmanCtrls ms_pacman)
{
   uint8_t port_mask = 0;
   
   //Turn all the relays off
   nrf_gpio_port_set(GPIO_RELAY_PORT, 0xFF);
   
   switch(ms_pacman.arrow_direction)
   {
      case ARROW_UP:
         port_mask |= MASK_ARROW_UP;
         break;
      case ARROW_DOWN:
         port_mask |= MASK_ARROW_DOWN;
         break;
      case ARROW_LEFT:
         port_mask |= MASK_ARROW_LEFT;
         break;
      case ARROW_RIGHT:
         port_mask |= MASK_ARROW_RIGHT;
         break;
   }
   
   if(ms_pacman.button_a)
   {
      port_mask |= MASK_BUTTON_A;
   }
   
   if(ms_pacman.button_b)
   {
      port_mask |= MASK_BUTTON_B;
   }
   
   //Clear the appropriate pins to turn the relays on
   nrf_gpio_port_clear(GPIO_RELAY_PORT, port_mask);
}

/*Initialize the Ms. Pacman Service*/
uint32_t ble_ms_pacman_init(ble_ms_pacman_t *p_ms_pacman)
{
   uint32_t   err_code;
   ble_uuid_t ble_uuid;

   // Initialize service structure
   p_ms_pacman->conn_handle = BLE_CONN_HANDLE_INVALID;
   
   // Add IMD service
   ble_uuid128_t base_uuid = {BLE_MS_PACMAN_BASE_UUID};
   err_code = sd_ble_uuid_vs_add(&base_uuid, &p_ms_pacman->uuid_type);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }
   
   ble_uuid.type = p_ms_pacman->uuid_type;
   ble_uuid.uuid = BLE_MS_PACMAN_SERVICE_UUID;
   
   err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_ms_pacman->service_handle);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }

   err_code = ms_pacman_char_add(p_ms_pacman);
   if (err_code != NRF_SUCCESS)
   {
      return err_code;
   }
    
   return NRF_SUCCESS;
}

/*Initialize the Ms. Pacman Characteristic*/
uint32_t ms_pacman_char_add(ble_ms_pacman_t *p_ms_pacman)
{
   ble_gatts_char_md_t char_md;
   ble_gatts_attr_t    attr_char_value;
   ble_uuid_t          ble_uuid;
   ble_gatts_attr_md_t attr_md;

   memset(&char_md, 0, sizeof(char_md));
   
   char_md.char_props.read   = 1;
   char_md.char_props.write  = 1;
   char_md.p_char_user_desc  = NULL;
   char_md.p_char_pf         = NULL;
   char_md.p_user_desc_md    = NULL;
   char_md.p_cccd_md         = NULL;
   char_md.p_sccd_md         = NULL;
   
   ble_uuid.type = p_ms_pacman->uuid_type;
   ble_uuid.uuid = BLE_MS_PACMAN_CHARACTERISTIC_UUID;
   
   memset(&attr_md, 0, sizeof(attr_md));
   
   BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
   BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
   attr_md.vloc       = BLE_GATTS_VLOC_STACK;
   attr_md.rd_auth    = 0;
   attr_md.wr_auth    = 0;
   attr_md.vlen       = 0;
   
   memset(&attr_char_value, 0, sizeof(attr_char_value));
   
   attr_char_value.p_uuid       = &ble_uuid;
   attr_char_value.p_attr_md    = &attr_md;
   attr_char_value.init_len     = BLE_MS_PACMAN_CHARACTERISTIC_SIZE;
   attr_char_value.init_offs    = 0;
   attr_char_value.max_len      = BLE_MS_PACMAN_CHARACTERISTIC_SIZE;
   attr_char_value.p_value      = NULL;
   
   return sd_ble_gatts_characteristic_add(p_ms_pacman->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_ms_pacman->ms_pacman_char_handles);
}