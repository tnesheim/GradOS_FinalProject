#include <stdint.h>

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
   } while(memcmp(rcvBuf, NRF_BLE_INIT_SUCESS, 6) == 0); 
}
