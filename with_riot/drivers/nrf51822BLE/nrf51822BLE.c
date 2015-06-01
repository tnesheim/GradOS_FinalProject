#include <stdint.h>
#include <string.h>

#include "kernel.h"
#include "mutex.h"
#include "periph/spi.h"
#include "nrf51822BLE.h"
#include "transceiver.h"
#include "hwtimer.h"

/*Initialize the NRF BLE transceiver w/ the RIOT OS*/
void nrf51822ble_init(void)
{
   //Initialize the hwtimer if not already started
   if(hwtimer_active() != 1)
   {
      hwtimer_init();
   }

   //Wait for 500ms to give the nrf time to init
   hwtimer_wait(HWTIMER_TICKS(500000));

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

   //Tell the NRF that we want to receive a pkt
   uint8_t cmd_byte = RCV_PKT_NRF51822BLE;
   spi_transfer_bytes(SPI_0, &cmd_byte, NULL, 1);

   //Wait for a little bit to ensure the NRF has time to 
   //load the data, 2ms
   hwtimer_wait(HWTIMER_TICKS(2000));

   //Receive ble packet
   spi_transfer_bytes(SPI_0, NULL, rcv_buf, NRF51822_SPI_PKT_LEN);

   //Copy the data over to the radio pkt buffer
   blePkt->msg_type     = rcv_buf[NRF51822_SPI_MSG_TYPE_OFFSET];
   blePkt->src_address  = rcv_buf[NRF51822_SPI_SRC_OFFSET];
   blePkt->dest_address = rcv_buf[NRF51822_SPI_DST_OFFSET];
   //Copy the payload over
   memcpy(blePkt->payload, &(rcv_buf[NRF51822_SPI_PAYLOAD_OFFSET]), NRF51822_MAX_DATA_LENGTH);
}

/*Sends the current BLE radio pkt*/
void nrfSendPkt(ble_radio_pkt *blePkt)
{
   uint8_t send_buf[NRF51822_SPI_PKT_LEN];

   //Fill in the data that is to be sent
   send_buf[NRF51822_SPI_MSG_TYPE_OFFSET] = blePkt->msg_type;
   send_buf[NRF51822_SPI_SRC_OFFSET]      = blePkt->src_address;
   send_buf[NRF51822_SPI_DST_OFFSET]      = blePkt->dest_address;
   memcpy(&(send_buf[NRF51822_SPI_PAYLOAD_OFFSET]), blePkt->payload, NRF51822_MAX_DATA_LENGTH);  

   //Send the data to the NRF device
   spi_transfer_bytes(SPI_0, send_buf, NULL, NRF51822_SPI_PKT_LEN);
}
