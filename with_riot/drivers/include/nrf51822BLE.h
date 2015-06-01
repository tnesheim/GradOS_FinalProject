#ifndef NRF51822_BLE_H_
#define NRF51822_BLE_H_

#include <stdint.h>
  
#include "kernel.h"
#include "mutex.h"
#include "periph/spi.h"

#define NRF51822_MAX_DATA_LENGTH (64)
#define NRF51822_BROADCAST_ADDRESS (0xFFFF)

#define NRF_BLE_INIT_STR     "INIT"
#define NRF_BLE_INIT_SUCCESS "GOOD"


/*Initialize the NRF BLE transceiver w/ the RIOT OS*/
void nrf51822ble_init(void);

/*Initialize the spi connection and send the init pkt*/
void nrfInitSPI(void);

#endif 
