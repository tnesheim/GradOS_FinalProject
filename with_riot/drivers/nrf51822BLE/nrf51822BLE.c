#include <stdint.h>
#include <string.h>

#include "kernel.h"
#include "mutex.h"
#include "periph/spi.h"
#include "nrf51822BLE.h"

/*Initialize the NRF BLE transceiver w/ the RIOT OS*/
void nrf51822ble_init(void)
{
   //Init the SPI and send the 'INIT' pkt. 
   nrfInitSPI();
}

/*Initialize the spi connection and send the init pkt*/
void nrfInitSPI(void)
{
   char rcvBuf[6]; 

   //Initialize the SPI as Master
   spi_init_master(SPI_0, SPI_CONF_FIRST_RISING, SPI_SPEED_1MHZ);     
   
   //Send the 'INIT' pkt. to start BLE communication
   //Keep sending until "GOOD" is returned
   do
   {
      spi_transfer_bytes(SPI_0, NRF_BLE_INIT_STR, rcvBuf, 4);   
   } while(memcmp(rcvBuf, NRF_BLE_INIT_SUCCESS, 4) != 0); 
}

/*Receives the current BLE radio pkt*/
void nrfRcvPkt(ble_radio_pkt * blePkt)
{
   uint8_t rcv_buf[NRF51822_SPI_PKT_LEN];

   //Receive ble packets
   spi_transfer_bytes(SPI_0, NULL, rcv_buf, NRF51822_SPI_PKT_LEN);

   //Copy the data over to the radio pkt buffer
   blePkt->src_address  = rcv_buf[NRF51822_SPI_SRC_OFFSET];
   blePkt->dest_address = rcv_buf[NRF51822_SPI_DST_OFFSET];
   //Copy the payload over
   memcpy(blePkt->payload, &(rcv_buf[NRF51822_SPI_PAYLOAD_OFFSET]), NRF51822_MAX_DATA_LENGTH);
}
