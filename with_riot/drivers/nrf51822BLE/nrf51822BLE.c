#include <stdint.h>
#include <string.h>

#include "kernel.h"
#include "mutex.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "nrf51822BLE.h"
#include "transceiver.h"
#include "hwtimer.h"
#include "periph/gpio.h"

static kernel_pid_t trans_pid;
ble_radio_pkt nrf51822_blePkt;

/*Initialize the NRF BLE transceiver w/ the RIOT OS*/
void nrf51822ble_init(kernel_pid_t transceiver_pid)
{
   //Store the PID of the transceiver 
   trans_pid = transceiver_pid;   

   //Initialize the hwtimer if not already started
   if(hwtimer_active() != 1)
   {
      hwtimer_init();
   }
}

/*Initialize the spi connection and send the init pkt*/
void nrfInitSPI(void)
{
   //Init CSN and set it to high
   gpio_init_out(GPIO_0, GPIO_PULLUP);   
   gpio_set(GPIO_0);

   //Initialize the SPI as Master
   spi_init_master(SPI_0, SPI_CONF_FIRST_RISING, SPI_SPEED_1MHZ);     
}

/*Initializes the RX interrupt GPIO pin*/
void nrfInitRxInterrupt(void)
{
   //Set the interrupt for falling edge to indicate a new RX pkt.
   gpio_init_int(GPIO_1, GPIO_PULLUP, GPIO_FALLING, nrfRcvPkt, NULL);    
}

/*Locks the SPI bus and transfers data.*/
void nrfSPITransfer(char* tx_buf, char* rx_buf, int length)
{
   //Acquire exclusive SPI access
   spi_acquire(SPI_0);

   //Clear CS and transfer bytes
   gpio_clear(GPIO_0);
   spi_transfer_bytes(SPI_0, tx_buf, rx_buf, NRF51822_SPI_PKT_LEN);
   gpio_set(GPIO_0);

   //Release the SPI
   spi_release(SPI_0); 
}

/*Receives the current BLE radio pkt*/
void nrfRcvPkt(void *arg)
{
   uint8_t rcv_buf[NRF51822_SPI_PKT_LEN];

   //Receive ble packet
   nrfSPITransfer(NULL, rcv_buf, NRF51822_SPI_PKT_LEN);

   //Copy the data over to the radio pkt buffer
   nrf51822_blePkt.msg_type     = rcv_buf[NRF51822_SPI_MSG_TYPE_OFFSET];
   nrf51822_blePkt.src_address  = rcv_buf[NRF51822_SPI_SRC_OFFSET_0];
   nrf51822_blePkt.src_address  = (nrf51822_blePkt.src_address << 8) & rcv_buf[NRF51822_SPI_SRC_OFFSET_1];
   nrf51822_blePkt.dest_address = rcv_buf[NRF51822_SPI_DST_OFFSET_0];
   nrf51822_blePkt.dest_address = (nrf51822_blePkt.dest_address << 8) & rcv_buf[NRF51822_SPI_DST_OFFSET_1];
   //Copy the payload over
   memcpy(nrf51822_blePkt.payload, &(rcv_buf[NRF51822_SPI_PAYLOAD_OFFSET]), NRF51822_MAX_DATA_LENGTH);

   //Assuming it exists, send the RX pkt. to the transceiver thread
   if(trans_pid != KERNEL_PID_UNDEF)
   {
      msg_t m;
      m.type = RCV_PKT_NRF51822BLE;
      m.content.value = 0;
      msg_send(&m, trans_pid);
   }
}

/*Sends the current BLE radio pkt*/
void nrfSendPkt(ble_radio_pkt *blePkt)
{
   uint8_t send_buf[NRF51822_SPI_PKT_LEN];

   //Fill in the data that is to be sent
   send_buf[NRF51822_SPI_MSG_TYPE_OFFSET] = blePkt->msg_type;
   send_buf[NRF51822_SPI_SRC_OFFSET_0]    = (blePkt->src_address & 0xFF00) >> 8;
   send_buf[NRF51822_SPI_SRC_OFFSET_1]    = blePkt->src_address & 0x00FF;
   send_buf[NRF51822_SPI_DST_OFFSET_0]    = (blePkt->dest_address & 0xFF00) >> 8;
   send_buf[NRF51822_SPI_DST_OFFSET_1]    = blePkt->dest_address & 0x00FF;
   memcpy(&(send_buf[NRF51822_SPI_PAYLOAD_OFFSET]), blePkt->payload, NRF51822_MAX_DATA_LENGTH);  

   //Send the data to the NRF/Bluegiga device
   nrfSPITransfer(send_buf, NULL, NRF51822_SPI_PKT_LEN);
}
