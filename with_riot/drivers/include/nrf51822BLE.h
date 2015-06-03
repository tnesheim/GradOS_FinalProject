#ifndef NRF51822_BLE_H_
#define NRF51822_BLE_H_

#include <stdint.h>
  
#include "kernel.h"
#include "mutex.h"
#include "periph/spi.h"
#include "transceiver.h"

#define NRF51822_MAX_DATA_LENGTH (20)
#define NRF51822_BROADCAST_ADDRESS (0xFFFF)

#define NRF51822_SPI_HDR_LEN 5
#define NRF51822_SPI_PKT_LEN NRF51822_SPI_HDR_LEN + NRF51822_MAX_DATA_LENGTH
#define NRF51822_SPI_MSG_TYPE_OFFSET 0
#define NRF51822_SPI_SRC_OFFSET_0    1
#define NRF51822_SPI_SRC_OFFSET_1    2 
#define NRF51822_SPI_DST_OFFSET_0    3 
#define NRF51822_SPI_DST_OFFSET_1    4 
#define NRF51822_SPI_PAYLOAD_OFFSET  5 

#define NRF_BLE_INIT_STR     "INIT"
#define NRF_BLE_INIT_SUCCESS "GOOD"

//BLE Radio pkt
typedef struct ble_radio_pkt {
   uint8_t  msg_type;
   uint16_t src_address;
   uint16_t dest_address;
   uint8_t payload[NRF51822_MAX_DATA_LENGTH];
} ble_radio_pkt;

/*Initialize the NRF BLE transceiver w/ the RIOT OS*/
void nrf51822ble_init(void);

/*Initialize the spi connection*/
void nrfInitSPI(void);

/*Initialize the uart connection*/
void nrfInitUART(void);

/*Receives a pkt from the BLE Transceiver*/
void nrfRxUART(uint8_t *rxBuf);

/*Send a pkt to the BLE Transceiver*/
void nrfTxUART(uint8_t *txBuf);

/*Receives the current BLE radio pkt*/
void nrfRcvPkt(ble_radio_pkt *blePkt);

/*Sends the current BLE radio pkt*/
void nrfSendPkt(ble_radio_pkt *blePkt);

#endif 
