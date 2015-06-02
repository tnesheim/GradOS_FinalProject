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
#include "transceiver_service.h"
#include "nrf51822BLE.h"

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
    uint8_t                      char_index_rx;     /**< Client characteristics index in discovered service information. */
    uint8_t                      char_index_tx;     /**< Client characteristics index in discovered service information. */
    uint8_t                      state;             /**< Client state. */
    uint8_t                      tx_buf[NRF51822_PKT_LEN];
} client_t;

static client_t         m_client[MAX_CLIENTS];      /**< Client context information list. */
static uint8_t          m_client_count;             /**< Number of clients. */
static uint8_t          m_base_uuid_type_transceiver;           /**< UUID type. */

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

static uint32_t client_find_addr(uint8_t *addr)
{
   uint32_t i;
   ble_gap_addr_t ble_addr;
   
   for (i = 0; i < MAX_CLIENTS; i++)
   {
      dm_peer_addr_get(&m_client[i].handle, &ble_addr);
      
      if(addr[0] == ble_addr.addr[5] && addr[1] == ble_addr.addr[4])
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
    write_params.handle   = p_client->srv_db.services[1].charateristics[p_client->char_index_tx].cccd_handle;
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

            //Check if this is the RX or the TX characteristic
            if ((p_characteristic->characteristic.uuid.uuid == TRANSCEIVER_RX_CHARACTERISTIC_UUID)
                &&
                (p_characteristic->characteristic.uuid.type == m_base_uuid_type_transceiver))
            {
                // Characteristic found. Store the information needed and break.

                p_client->char_index_rx = i;
            }
            //See if this is the Nunchuck service
            else if ((p_characteristic->characteristic.uuid.uuid == TRANSCEIVER_TX_CHARACTERISTIC_UUID)
                     &&
                     (p_characteristic->characteristic.uuid.type == m_base_uuid_type_transceiver))
            {
                // Characteristic found. Store the information needed and break.
                p_client->char_index_tx = i;
                is_valid_srv_found   = true;
            }
        }
    }

    //Only enable notifications if it is the TX characteristic
    if (is_valid_srv_found)
    {
        // Enable notification.
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
            p_client->srv_db.services[1].charateristics[p_client->char_index_tx].cccd_handle &&
            p_ble_evt->evt.gattc_evt.params.write_rsp.handle !=
            p_client->srv_db.services[1].charateristics[p_client->char_index_rx].cccd_handle)
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
            p_client->srv_db.services[1].charateristics[p_client->char_index_tx].characteristic.handle_value)
            && (p_ble_evt->evt.gattc_evt.params.hvx.len == NRF51822_PKT_LEN))
        {
           //Store the received Transceiver TX data in the TX buffer 
           memcpy(p_client->tx_buf, p_ble_evt->evt.gattc_evt.params.hvx.data, NRF51822_PKT_LEN);
        }
    }
}

/*Get what is currently in the TX buffer and copy it into the Transceiver RX buffer*/
void tx_get(uint8_t *addr, uint8_t * rx_buf)
{
   uint32_t node_ndx = client_find_addr(addr);
   
   //Don't do anything if the Transceiver client wasn't found
   if(node_ndx == MAX_CLIENTS)
   {
      return;
   }
   
   //Copy the data 
   memcpy(rx_buf, m_client[node_ndx].tx_buf, NRF51822_PKT_LEN);
}

/*Send the received RX data to the node*/
void rx_send(uint8_t *rx_buf)
{
   ble_gattc_write_params_t write_params = {0};
   
   //Find the Ms. Pacman node
   uint32_t node_ndx = client_find_addr(&(rx_buf[NRF51822_SPI_DST_OFFSET_0]));
   
   //Didn't find the transceiver address, return
   if(node_ndx == MAX_CLIENTS)
   {
      return;
   }
   
   //Fill in the buffer with the appropriate values
   
   /* Central writes to CCCD of peripheral to receive indications */
   write_params.write_op = BLE_GATT_OP_WRITE_REQ;
   write_params.handle = m_client[node_ndx].srv_db.services[0].charateristics[m_client[node_ndx].char_index_rx].characteristic.handle_value;
   write_params.offset = 0;
   write_params.len = NRF51822_PKT_LEN;
   write_params.p_value = rx_buf;
   
   sd_ble_gattc_write(m_client[node_ndx].srv_db.conn_handle, &write_params);
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

    //Add the Transceiver base UUID
    ble_uuid128_t base_uuid = TRANSCEIVER_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_base_uuid_type_transceiver);
    APP_ERROR_CHECK(err_code);
   
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        m_client[i].state  = IDLE;
    }

    m_client_count = 0;

    db_discovery_init();

    // Register with discovery module for the discovery of the service.
    ble_uuid_t uuid;

    //Register the Transceiver Service with the discover module
    uuid.type = m_base_uuid_type_transceiver;
    uuid.uuid = TRANSCEIVER_SERVICE_UUID;

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
uint32_t client_handling_create(const dm_handle_t * p_handle, uint16_t conn_handle)
{
    m_client[p_handle->connection_id].state              = STATE_SERVICE_DISC;
    m_client[p_handle->connection_id].srv_db.conn_handle = conn_handle;
    m_client_count++;
    m_client[p_handle->connection_id].handle             = (*p_handle);
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

