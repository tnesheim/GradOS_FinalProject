#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_db_discovery.h"
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "ble_advdata_parser.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "app_util.h"
#include "client_handling.h"
#include "spi_slave.h"
#include "transceiver.h"
#include "nrf51822BLE.h"
#include "transceiver_service.h"

#define MISO_PIN     1
#define MOSI_PIN     2
#define SCK_PIN      3
#define CS_PIN       4
#define RX_INTERRUPT 5

#define SEC_PARAM_BOND             1                                  /**< Perform bonding. */
#define SEC_PARAM_MITM             0                                  /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES  BLE_GAP_IO_CAPS_NONE               /**< No I/O capabilities. */
#define SEC_PARAM_OOB              0                                  /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE     7                                  /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE     16                                 /**< Maximum encryption key size. */

#define SCAN_INTERVAL              0x00A0                             /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                0x0050                             /**< Determines scan window in units of 0.625 millisecond. */

#define MIN_CONNECTION_INTERVAL    MSEC_TO_UNITS(7.5, UNIT_1_25_MS)   /**< Determines maximum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL    MSEC_TO_UNITS(30, UNIT_1_25_MS)    /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY              0                                  /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT        MSEC_TO_UNITS(4000, UNIT_10_MS)    /**< Determines supervision time-out in units of 10 millisecond. */

#define MAX_PEER_COUNT DEVICE_MANAGER_MAX_CONNECTIONS

#define MS_PACMAN_CONNECT_LED LED_0
#define NUNCHUCK_CONNECT_LED  LED_1

typedef enum
{
    BLE_NO_SCAN,                                                  /**< No advertising running. */
    BLE_WHITELIST_SCAN,                                           /**< Advertising with whitelist. */
    BLE_FAST_SCAN,                                                /**< Fast advertising running. */
} ble_advertising_mode_t;

//SPI Variables
static uint8_t rx_buf[NRF51822_PKT_LEN];
static uint8_t tx_buf[NRF51822_PKT_LEN];
static bool spi_master_recv = false;
extern uint8_t m_hvx_buffer[];

//The peer addresses of the Ms. Pacman and Nunchuck Nodes
//Ms. Pacman: 0xFE11F5A2E383
static uint8_t m_peer_addr_ms_pacman[] = {0x83, 0xE3, 0xA2, 0xF5, 0x11, 0xFE};
//Nunchuck: 0xCE1B2093407F
static uint8_t m_peer_addr_nunchuck[]  = {0x7F, 0x40, 0x93, 0x20, 0x1B, 0xCE};   

static ble_db_discovery_t           m_ble_db_discovery;                  /**< Structure used to identify the DB Discovery module. */
static ble_gap_scan_params_t        m_scan_param;                        /**< Scan parameters requested for scanning and connection. */
static dm_application_instance_t    m_dm_app_id;                         /**< Application identifier. */
static dm_handle_t                  m_dm_device_handle;                  /**< Device Identifier identifier. */
static uint8_t                      m_peer_count = 0;                    /**< Number of peer's connected. */
static uint8_t                      m_scan_mode;                         /**< Scan mode used by application. */

static bool                         m_memory_access_in_progress = false; /**< Flag to keep track of ongoing operations on persistent memory. */

/**
 * @brief Connection parameters requested for connection.
 */
static const ble_gap_conn_params_t m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,   // Minimum connection
    (uint16_t)MAX_CONNECTION_INTERVAL,   // Maximum connection
    0,                                   // Slave latency
    (uint16_t)SUPERVISION_TIMEOUT        // Supervision time-out
};

static void scan_start(void);
static void initGPIO();
static void setLed(uint32_t led_pin, bool set_led);

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    
    setLed(MS_PACMAN_CONNECT_LED, true);

    NVIC_SystemReset();
}


/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Callback handling device manager events.
 *
 * @details This function is called to notify the application of device manager events.
 *
 * @param[in]   p_handle      Device Manager Handle. For link related events, this parameter
 *                            identifies the peer.
 * @param[in]   p_event       Pointer to the device manager event.
 * @param[in]   event_status  Status of the event.
 */
static api_result_t device_manager_event_handler(const dm_handle_t    * p_handle,
                                                 const dm_event_t     * p_event,
                                                 const api_result_t     event_result)
{
    bool     lcd_write_status;
    uint32_t err_code;

    switch(p_event->event_id)
    {
        case DM_EVT_CONNECTION:
        {
            m_dm_device_handle = (*p_handle);
            
            //Create the client
            err_code = client_handling_create(p_handle, p_event->event_param.p_gap_param->conn_handle);
            APP_ERROR_CHECK(err_code); 
           
            m_peer_count++;
            if (m_peer_count < MAX_PEER_COUNT)
            {
                scan_start();
            }
            break;
        }
        
        case DM_EVT_DISCONNECTION:
        {
            memset(&m_ble_db_discovery, 0 , sizeof (m_ble_db_discovery));

            err_code = client_handling_destroy(p_handle);
            APP_ERROR_CHECK(err_code);
            
            if (m_peer_count == MAX_PEER_COUNT)
            {
                scan_start();
            }
            m_peer_count--;
            break;
        }
        
        case DM_EVT_SECURITY_SETUP:
        {
            // Slave securtiy request received from peer, if from a non bonded device, 
            // initiate security setup, else, wait for encryption to complete.
            err_code = dm_security_setup_req(&m_dm_device_handle);
            APP_ERROR_CHECK(err_code);
            break;
        }
        case DM_EVT_SECURITY_SETUP_COMPLETE:
        {      
            break;
        }
        
        case DM_EVT_LINK_SECURED:
            break;
            
        case DM_EVT_DEVICE_CONTEXT_LOADED:
            APP_ERROR_CHECK(event_result);
            break;
            
        case DM_EVT_DEVICE_CONTEXT_STORED:
            APP_ERROR_CHECK(event_result);
            break;
            
        case DM_EVT_DEVICE_CONTEXT_DELETED:
            APP_ERROR_CHECK(event_result);
            break;
            
        default:
            break;
    }

    // Relay the event to client handling module.
    err_code = client_handling_dm_event_handler(p_handle, p_event, event_result);
    APP_ERROR_CHECK(err_code);
    
    return NRF_SUCCESS;
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                err_code;
    const ble_gap_evt_t   * p_gap_evt = &p_ble_evt->evt.gap_evt;
   
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
        {
           ble_gap_evt_adv_report_t adv_report = p_ble_evt->evt.gap_evt.params.adv_report;
           
           uint8_t base_uuid[] = TRANSCEIVER_SERVICE_UUID_128;
           
           //Check the adv. pkt. to see if it contains the base Transceiver UUID
           if(memcmp(base_uuid, &(adv_report.data[5]), 16) == 0)
           {
              //Attempt a connection
              sd_ble_gap_connect(&p_ble_evt->evt.gap_evt.params.adv_report.peer_addr, &m_scan_param, &m_connection_param);
           }
           break;
        }
        case BLE_GAP_EVT_TIMEOUT:
            break;
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
static void on_sys_evt(uint32_t sys_evt)
{
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                scan_start();
            }
            break;
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *  been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    client_handling_ble_evt_handler(p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    ble_enable_params_t ble_enable_params;
   
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    //Enable the BLE stack
    ble_enable_params.gap_enable_params.role = BLE_GAP_ROLE_CENTRAL;
    ble_enable_params.gatts_enable_params.service_changed = 1;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code); 
   
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Device Manager.
 *
 * @details Device manager is initialized here.
 */
static void device_manager_init(void)
{
    dm_application_param_t param;
    dm_init_param_t        init_param;

    uint32_t              err_code;

    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    // Clear all bonded devices if user requests to.
    init_param.clear_persistent_data = true;

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&param.sec_param, 0, sizeof (ble_gap_sec_params_t));

    // Event handler to be registered with the module.
    param.evt_handler            = device_manager_event_handler;

    // Service or protocol context for device manager to load, store and apply on behalf of application.
    // Here set to client as application is a GATT client.
    param.service_type           = DM_PROTOCOL_CNTXT_GATT_CLI_ID;

    // Secuirty parameters to be used for security procedures.
    param.sec_param.bond         = SEC_PARAM_BOND;
    param.sec_param.mitm         = SEC_PARAM_MITM;
    param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    param.sec_param.oob          = SEC_PARAM_OOB;
    param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    param.sec_param.kdist_periph.enc = 1;
    param.sec_param.kdist_periph.id  = 1;

    err_code = dm_register(&m_dm_app_id,&param);
    APP_ERROR_CHECK(err_code);
}

/**@breif Function to start scanning.
 */
static void scan_start(void)
{
    ble_gap_whitelist_t   whitelist;
    ble_gap_addr_t        * p_whitelist_addr[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    ble_gap_irk_t         * p_whitelist_irk[BLE_GAP_WHITELIST_IRK_MAX_COUNT];
    uint32_t              err_code;
    uint32_t              count;

    // Verify if there is any flash access pending, if yes delay starting scanning until 
    // it's complete.
    err_code = pstorage_access_status_get(&count);
    APP_ERROR_CHECK(err_code);
    
    if (count != 0)
    {
        m_memory_access_in_progress = true;
        return;
    }
    
    // Initialize whitelist parameters.
    whitelist.addr_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
    whitelist.irk_count  = 0;
    whitelist.pp_addrs   = p_whitelist_addr;
    whitelist.pp_irks    = p_whitelist_irk;

    // Request creating of whitelist.
    err_code = dm_whitelist_create(&m_dm_app_id,&whitelist);
    APP_ERROR_CHECK(err_code);

    if (((whitelist.addr_count == 0) && (whitelist.irk_count == 0)) ||
         (m_scan_mode != BLE_WHITELIST_SCAN))
    {
        // No devices in whitelist, hence non selective performed.
        m_scan_param.active       = 0;            // Active scanning set.
        m_scan_param.selective    = 0;            // Selective scanning not set.
        m_scan_param.interval     = SCAN_INTERVAL;// Scan interval.
        m_scan_param.window       = SCAN_WINDOW;  // Scan window.
        m_scan_param.p_whitelist  = NULL;         // No whitelist provided.
        m_scan_param.timeout      = 0x0000;       // No timeout.
    }
    else
    {
        // Selective scanning based on whitelist first.
        m_scan_param.active       = 0;            // Active scanning set.
        m_scan_param.selective    = 1;            // Selective scanning not set.
        m_scan_param.interval     = SCAN_INTERVAL;// Scan interval.
        m_scan_param.window       = SCAN_WINDOW;  // Scan window.
        m_scan_param.p_whitelist  = &whitelist;   // Provide whitelist.
        m_scan_param.timeout      = 0x001E;       // 30 seconds timeout.

        // Set whitelist scanning state.
        m_scan_mode = BLE_WHITELIST_SCAN;
    }

    err_code = sd_ble_gap_scan_start(&m_scan_param);
    APP_ERROR_CHECK(err_code);
}

static void initGPIO()
{
   nrf_gpio_cfg_output(MS_PACMAN_CONNECT_LED);
   nrf_gpio_cfg_output(NUNCHUCK_CONNECT_LED);
   nrf_gpio_pin_clear(MS_PACMAN_CONNECT_LED);
   nrf_gpio_pin_clear(NUNCHUCK_CONNECT_LED);
   
   //Set RX Interrupt as output
   nrf_gpio_cfg_output(RX_INTERRUPT);
   nrf_gpio_pin_set(RX_INTERRUPT);
}
static void setLed(uint32_t led_pin, bool set_led)
{
   if(set_led)
   {
      nrf_gpio_pin_set(led_pin);
   }
   else
   {
      nrf_gpio_pin_clear(led_pin);
   }
}

void spi_event_handler(spi_slave_evt_t event)
{
   uint8_t type;
   
   switch(event.evt_type)
   {
      case SPI_SLAVE_BUFFERS_SET_DONE:
         break;
      case SPI_SLAVE_XFER_DONE:
         if(rx_buf[NRF51822_SPI_MSG_TYPE_OFFSET] == SND_PKT)
         {
            
            //Send the RX pkt to the specified address 
            rx_send(rx_buf);
            type = rx_buf[NRF51822_SPI_MSG_TYPE_OFFSET];
         }
         else if(spi_master_recv)
         {
            nrf_gpio_pin_set(RX_INTERRUPT);
            spi_master_recv = false;
         }
         
         //Setup the SPI buffers
         spi_slave_buffers_set(tx_buf, rx_buf, NRF51822_PKT_LEN, NRF51822_PKT_LEN);
         break;
   }
   
   
}

/*Initializes the slave SPI driver including register the event handler*/
static void initSPI()
{
   uint32_t           err_code;
   spi_slave_config_t spi_slave_config;
   
   //Initialize the SPI
    spi_slave_config.pin_miso         = MISO_PIN;
    spi_slave_config.pin_mosi         = MOSI_PIN;
    spi_slave_config.pin_sck          = SCK_PIN;
    spi_slave_config.pin_csn          = CS_PIN;
    spi_slave_config.mode             = SPI_MODE_0;
    spi_slave_config.bit_order        = SPIM_MSB_FIRST;
    spi_slave_config.def_tx_character = '0';
    spi_slave_config.orc_tx_character = '0';
   
   //Register the SPI Event Handler
   err_code = spi_slave_evt_handler_register(spi_event_handler);                                 
   APP_ERROR_CHECK(err_code);
   
   //Configure the SPI interface                                    
   err_code = spi_slave_init(&spi_slave_config);
   APP_ERROR_CHECK(err_code);
   
   //Setup the SPI buffers
   spi_slave_buffers_set(tx_buf, rx_buf, NRF51822_PKT_LEN, NRF51822_PKT_LEN);
}

/*This function is called when a Notify operation is received from a BLE client.*/
void rx_handler(uint8_t *rx_buffer)
{
   memcpy(tx_buf, m_hvx_buffer, NRF51822_PKT_LEN); 
   //Setup the SPI buffers
   spi_slave_buffers_set(tx_buf, rx_buf, NRF51822_PKT_LEN, NRF51822_PKT_LEN);
   
   spi_master_recv = true;
   
   //Notify the RIOT OS that data is available
   nrf_gpio_pin_clear(RX_INTERRUPT);
}

int main(void)
{
   initGPIO();
   initSPI();
   
   ble_stack_init();
   client_handling_init(rx_handler);
   device_manager_init();
   
   //Scan for peripherals that have the Transceiver UUID we are interested in
   scan_start();
   
   //SPI RX/TX from the Transceiver[s] happening in interrupts
   for(;;)
   {
   }
}
