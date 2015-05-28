/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

#include "client_handling.h"
#include <string.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "app_trace.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "ms_pacman_service.h"
#include "nunchuck_service.h"
#include "nunchuck.h"

/**@brief Client states. */
typedef enum
{
    IDLE,                                           /**< Idle state. */
    STATE_SERVICE_DISC,                             /**< Service discovery state. */
    STATE_NOTIF_ENABLE,                             /**< State where the request to enable notifications is sent to the peer. . */
    STATE_RUNNING,                                  /**< Running state. */
    STATE_ERROR                                     /**< Error state. */
} client_state_t;

/**@brief Client context information. */
typedef struct
{
    ble_db_discovery_t           srv_db;            /**< The DB Discovery module instance associated with this client. */
    dm_handle_t                  handle;            /**< Device manager identifier for the device. */
    uint8_t                      char_index;        /**< Client characteristics index in discovered service information. */
    uint8_t                      state;             /**< Client state. */
    uint8_t                      node_type;         /**< Specifies whether this is a Nunchuck node or a Ms. Pacman node. */
} client_t;

static client_t         m_client[MAX_CLIENTS];      /**< Client context information list. */
static uint8_t          m_client_count;             /**< Number of clients. */
static uint8_t          m_base_uuid_type_ms_pacman;           /**< UUID type. */
static uint8_t          m_base_uuid_type_nunchuck;            /**< UUID type. */


/**@brief Function for finding client context information based on handle.
 *
 * @param[in] conn_handle  Connection handle.
 *
 * @return client context information or NULL upon failure.
 */
static uint32_t client_find(uint16_t conn_handle)
{
    uint32_t i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_client[i].srv_db.conn_handle == conn_handle)
        {
            return i;
        }
    }

    return MAX_CLIENTS;
}

/**@brief Function for finding client context information based on node type.
 *
 * @param[in] node_type  The type of the node.
 *
 * @return client context information or NULL upon failure.
 */
static uint32_t client_find_node(uint8_t node_type)
{
    uint32_t i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_client[i].node_type == node_type)
        {
            return i;
        }
    }

    return MAX_CLIENTS;
}

/**@brief Function for service discovery.
 *
 * @param[in] p_client Client context information.
 */
static void service_discover(client_t * p_client)
{
    uint32_t   err_code;

    p_client->state = STATE_SERVICE_DISC;

    err_code = ble_db_discovery_start(&(p_client->srv_db),
                                      p_client->srv_db.conn_handle);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling enabling notifications.
 *
 * @param[in] p_client Client context information.
 */
static void notif_enable(client_t * p_client)
{
    uint32_t                 err_code;
    ble_gattc_write_params_t write_params;
    uint8_t                  buf[BLE_CCCD_VALUE_LEN];

    p_client->state = STATE_NOTIF_ENABLE;

    buf[0] = BLE_GATT_HVX_NOTIFICATION;
    buf[1] = 0;

    write_params.write_op = BLE_GATT_OP_WRITE_REQ;
    write_params.handle   = p_client->srv_db.services[1].charateristics[p_client->char_index].cccd_handle;
    write_params.offset   = 0;
    write_params.len      = sizeof(buf);
    write_params.p_value  = buf;
    
    err_code = sd_ble_gattc_write(p_client->srv_db.conn_handle, &write_params);
    APP_ERROR_CHECK(err_code);
}


static void db_discovery_evt_handler(ble_db_discovery_evt_t * p_evt)
{
    // Find the client using the connection handle.
    client_t * p_client;
    uint32_t   index;
    bool       is_valid_srv_found = false;

    index = client_find(p_evt->conn_handle);
    p_client = &m_client[index];
   
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
    {
        uint8_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            ble_db_discovery_char_t * p_characteristic;

            p_characteristic = &(p_evt->params.discovered_db.charateristics[i]);

            //See if this is the Ms. Pacman service
            if ((p_characteristic->characteristic.uuid.uuid == BLE_MS_PACMAN_CHARACTERISTIC_UUID)
                &&
                (p_characteristic->characteristic.uuid.type == m_base_uuid_type_ms_pacman))
            {
                // Characteristic found. Store the information needed and break.

                p_client->char_index = i;
                break;
            }
            //See if this is the Nunchuck service
            else if ((p_characteristic->characteristic.uuid.uuid == BLE_NUNCHUCK_CHARACTERISTIC_UUID)
                &&
                (p_characteristic->characteristic.uuid.type == m_base_uuid_type_nunchuck))
            {
                // Characteristic found. Store the information needed and break.

                p_client->char_index = i;
                is_valid_srv_found   = true;
                break;
            }
        }
    }

    //Only enable notifications if it is the Nunchuck characteristic
    if (is_valid_srv_found)
    {
        // Enable notification. Maybe don't do this??
        notif_enable(p_client);
    }
}


/**@brief Function for setting client to the running state once write response is received.
 *
 * @param[in] p_ble_evt Event to handle.
 */
static void on_evt_write_rsp(ble_evt_t * p_ble_evt, client_t * p_client)
{
    if ((p_client != NULL) && (p_client->state == STATE_NOTIF_ENABLE))
    {
        if (p_ble_evt->evt.gattc_evt.params.write_rsp.handle !=
            p_client->srv_db.services[1].charateristics[p_client->char_index].cccd_handle)
        {
            // Got response from unexpected handle.
            p_client->state = STATE_ERROR;
        }
        else
        {
            p_client->state = STATE_RUNNING;
        }
    }
}


/**@brief Function for toggling LEDS based on received notifications.
 *
 * @param[in] p_ble_evt Event to handle.
 */
static void on_evt_hvx(ble_evt_t * p_ble_evt, client_t * p_client, uint32_t index)
{
    if ((p_client != NULL) && (p_client->state == STATE_RUNNING))
    {
        if ((p_ble_evt->evt.gattc_evt.params.hvx.handle ==
            p_client->srv_db.services[1].charateristics[p_client->char_index].characteristic.handle_value)
            && (p_ble_evt->evt.gattc_evt.params.hvx.len == BLE_NUNCHUCK_CHARACTERISTIC_SIZE))
        {
           //Transform the Nunchuck data into Ms. Pacman controls
           MsPacmanCtrls ms_pacman = nunchuck_transform(p_ble_evt->evt.gattc_evt.params.hvx.data);
           
           //Send the data to the MsPacman node
           sendToMsPacman(ms_pacman);
        }
    }
}

/*Transform the received nunchuck data into Ms. Pacman controls*/
static MsPacmanCtrls nunchuck_transform(uint8_t *nunchuck_data)
{
   NunchuckData nun_data;
   MsPacmanCtrls ms_pacman;
   
   //Extract the Nunchuck data
   nun_data.joystick_x = nunchuck_data[BLE_NUNCHUCK_OFFSET_JOY_X];
   nun_data.joystick_y = nunchuck_data[BLE_NUNCHUCK_OFFSET_JOY_Y];
   nun_data.button_c   = nunchuck_data[BLE_NUNCHUCK_OFFSET_BUTTON_C];
   nun_data.button_z   = nunchuck_data[BLE_NUNCHUCK_OFFSET_BUTTON_Z];
   memcpy(&(nun_data.accel_x), &nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_X], 2);
   memcpy(&(nun_data.accel_y), &nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_Y], 2);
   memcpy(&(nun_data.accel_z), &nunchuck_data[BLE_NUNCHUCK_OFFSET_ACCEL_Z], 2);
   
   //Set the Ms. Pacman buttons
   ms_pacman.button_a = (nun_data.button_z == NUN_BUTTON_PRESSED) ? true:false;
   ms_pacman.button_b = (nun_data.button_c == NUN_BUTTON_PRESSED) ? true:false;
   
   //Set the Ms. Pacman direction based on the Nunchuck accelerometer values
   
   //Check the cases where both directions are above the threshold first
   uint16_t x_diff, y_diff;
   //In the upper left quadrant
   if(nun_data.accel_x < NUN_ACCEL_THRESHOLD_NEG && nun_data.accel_y > NUN_ACCEL_THRESHOLD_POS)
   {
      x_diff = NUN_ACCEL_THRESHOLD_CTR - nun_data.accel_x;
      y_diff = nun_data.accel_y - NUN_ACCEL_THRESHOLD_CTR;
      
      ms_pacman.arrow_direction = (x_diff > y_diff) ? ARROW_LEFT:ARROW_UP;
   }
   //In the upper right quadrant
   else if(nun_data.accel_x > NUN_ACCEL_THRESHOLD_POS && nun_data.accel_y > NUN_ACCEL_THRESHOLD_POS)
   {
      x_diff = nun_data.accel_x - NUN_ACCEL_THRESHOLD_CTR;
      y_diff = nun_data.accel_y - NUN_ACCEL_THRESHOLD_CTR;
      
      ms_pacman.arrow_direction = (x_diff > y_diff) ? ARROW_RIGHT:ARROW_UP;
   }
   //In the lower left quadrant
   else if(nun_data.accel_x < NUN_ACCEL_THRESHOLD_NEG && nun_data.accel_y < NUN_ACCEL_THRESHOLD_NEG)
   {
      x_diff = NUN_ACCEL_THRESHOLD_CTR - nun_data.accel_x;
      y_diff = NUN_ACCEL_THRESHOLD_CTR - nun_data.accel_y;
      
      ms_pacman.arrow_direction = (x_diff > y_diff) ? ARROW_LEFT:ARROW_DOWN;
   }
   //In the lower right quadrant
   else if(nun_data.accel_x > NUN_ACCEL_THRESHOLD_POS && nun_data.accel_y < NUN_ACCEL_THRESHOLD_NEG)
   {
      x_diff = nun_data.accel_x - NUN_ACCEL_THRESHOLD_CTR;
      y_diff = NUN_ACCEL_THRESHOLD_CTR - nun_data.accel_y;
      
      ms_pacman.arrow_direction = (x_diff > y_diff) ? ARROW_RIGHT:ARROW_DOWN;
   }
   //Going up
   else if(nun_data.accel_y > NUN_ACCEL_THRESHOLD_POS)
   {
      ms_pacman.arrow_direction = ARROW_UP;
   }
   //Going down
   else if(nun_data.accel_y < NUN_ACCEL_THRESHOLD_NEG)
   {
      ms_pacman.arrow_direction = ARROW_DOWN;
   }
   //Going left
   else if(nun_data.accel_x < NUN_ACCEL_THRESHOLD_NEG)
   {
      ms_pacman.arrow_direction = ARROW_LEFT;
   }
   else if(nun_data.accel_x > NUN_ACCEL_THRESHOLD_POS)
   {
      ms_pacman.arrow_direction = ARROW_RIGHT;
   }
   
   return ms_pacman;
}

/*Send the updated ctrls to the Ms. Pacman node*/
static void sendToMsPacman(MsPacmanCtrls ms_pacman)
{
   ble_gattc_write_params_t write_params = {0};
   uint8_t ms_pacman_buf[BLE_MS_PACMAN_CHARACTERISTIC_SIZE];
   
   //Find the Ms. Pacman node
   uint32_t node_ndx = client_find_node(MS_PACMAN_NODE);
   client_t ms_pacman_node = m_client[node_ndx];
   
   //Don't do anything if Ms. Pacman isn't connected
   if(ms_pacman_node.state == IDLE)
   {
      return;
   }
   
   //Fill in the buffer with the appropriate values
   ms_pacman_buf[BLE_MS_PACMAN_OFFSET_ARROW]    = ms_pacman.arrow_direction;
   ms_pacman_buf[BLE_MS_PACMAN_OFFSET_BUTTON_A] = ms_pacman.button_a;
   ms_pacman_buf[BLE_MS_PACMAN_OFFSET_BUTTON_B] = ms_pacman.button_b;
   
   /* Central writes to CCCD of peripheral to receive indications */
   write_params.write_op = BLE_GATT_OP_WRITE_REQ;
   write_params.handle = ms_pacman_node.srv_db.services[0].charateristics[ms_pacman_node.char_index].characteristic.handle_value;
   write_params.offset = 0;
   write_params.len = BLE_MS_PACMAN_CHARACTERISTIC_SIZE;
   write_params.p_value = ms_pacman_buf;
   
   sd_ble_gattc_write(ms_pacman_node.srv_db.conn_handle, &write_params);
}


/**@brief Function for handling timeout events.
 */
static void on_evt_timeout(ble_evt_t * p_ble_evt, client_t * p_client)
{
    APP_ERROR_CHECK_BOOL(p_ble_evt->evt.gattc_evt.params.timeout.src
                         == BLE_GATT_TIMEOUT_SRC_PROTOCOL);

    if (p_client != NULL)
    {
        p_client->state = STATE_ERROR;
    }
}


api_result_t client_handling_dm_event_handler(const dm_handle_t    * p_handle,
                                              const dm_event_t     * p_event,
                                              const api_result_t     event_result)
{
    client_t * p_client = &m_client[p_handle->connection_id];

    switch (p_event->event_id)
    {
       case DM_EVT_LINK_SECURED:
           // Attempt configuring CCCD now that bonding is established.
           if (event_result == NRF_SUCCESS)
           {
               notif_enable(p_client); 
           }
           break;
       default:
           break;
    }

    return NRF_SUCCESS;
}


void client_handling_ble_evt_handler(ble_evt_t * p_ble_evt)
{
    client_t * p_client = NULL;

    uint32_t index = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if (index != MAX_CLIENTS)
    {
       p_client = &m_client[index];
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_WRITE_RSP:
            if ((p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION) ||
                (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_ENCRYPTION))
            {
                uint32_t err_code = dm_security_setup_req(&p_client->handle);
                APP_ERROR_CHECK(err_code);

            }
            on_evt_write_rsp(p_ble_evt, p_client);
            break;

        case BLE_GATTC_EVT_HVX:
            on_evt_hvx(p_ble_evt, p_client, index);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            on_evt_timeout(p_ble_evt, p_client);
            break;

        default:
            break;
    }


    if (p_client != NULL)
    {
        ble_db_discovery_on_ble_evt(&(p_client->srv_db), p_ble_evt);
    }
}


/**@brief Database discovery module initialization.
 */
static void db_discovery_init(void)
{
    uint32_t err_code = ble_db_discovery_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the client handling.
 */
void client_handling_init(void)
{
    uint32_t err_code;
    uint32_t i;

    //Add the Ms. Pacman base UUID
    ble_uuid128_t base_uuid = BLE_MS_PACMAN_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_base_uuid_type_ms_pacman);
    APP_ERROR_CHECK(err_code);

    //Add the Nunchuck base UUID
    ble_uuid128_t base_uuid_1 = BLE_NUNCHUCK_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid_1, &m_base_uuid_type_nunchuck);
    APP_ERROR_CHECK(err_code);
   
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        m_client[i].state  = IDLE;
    }

    m_client_count = 0;

    db_discovery_init();

    // Register with discovery module for the discovery of the service.
    ble_uuid_t uuid;

    //Register the Ms. Pacman Service with the discover module
    uuid.type = m_base_uuid_type_ms_pacman;
    uuid.uuid = BLE_MS_PACMAN_SERVICE_UUID;

    err_code = ble_db_discovery_evt_register(&uuid,
                                             db_discovery_evt_handler);

    //Register the Nunchuck Service with the discover module
    uuid.type = m_base_uuid_type_nunchuck;
    uuid.uuid = BLE_NUNCHUCK_SERVICE_UUID;

    err_code = ble_db_discovery_evt_register(&uuid,
                                             db_discovery_evt_handler);
    
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for returning the current number of clients.
 */
uint8_t client_handling_count(void)
{
    return m_client_count;
}


/**@brief Function for creating a new client.
 */
uint32_t client_handling_create(const dm_handle_t * p_handle, uint16_t conn_handle, uint8_t node_type)
{
    m_client[p_handle->connection_id].state              = STATE_SERVICE_DISC;
    m_client[p_handle->connection_id].srv_db.conn_handle = conn_handle;
                m_client_count++;
    m_client[p_handle->connection_id].handle             = (*p_handle);
    m_client[p_handle->connection_id].node_type          = node_type;
    service_discover(&m_client[p_handle->connection_id]);

    return NRF_SUCCESS;
}


/**@brief Function for freeing up a client by setting its state to idle.
 */
uint32_t client_handling_destroy(const dm_handle_t * p_handle)
{
    uint32_t      err_code = NRF_SUCCESS;
    client_t    * p_client = &m_client[p_handle->connection_id];

    if (p_client->state != IDLE)
    {
            m_client_count--;
            p_client->state = IDLE;
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    return err_code;
}

