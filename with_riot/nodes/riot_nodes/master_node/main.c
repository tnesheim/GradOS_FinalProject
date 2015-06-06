#include <stdio.h>
#include <string.h>
#include "board.h"
#include "kernel.h"
#include "thread.h"
#include "msg.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "transceiver.h"
#include "vtimer.h"
#include "hwtimer.h"
#include "nrf51822BLE.h"

#define NODE_ONE_ADDR 0xD92F
#define NODE_TWO_ADDR 0xD35F
#define MS_PACMAN_ADDR 0xD92F

static char rx_thread_stack[KERNEL_CONF_STACKSIZE_DEFAULT];

void setLED(msg_t *m)
{
    uint16_t src_addr;
    uint8_t led_data[NRF51822_MAX_DATA_LENGTH];
    uint8_t led_value;
    radio_packet_t *trans_buf;

    //Copy the data from the transceiver
    trans_buf = (radio_packet_t *)(m->content.ptr);
    memcpy(led_data, trans_buf->data, NRF51822_MAX_DATA_LENGTH);

    //Tell the Transceiver that we are done with the buffer
    trans_buf->processing = 0;

    //Extract the Source address from the rcv'd pkt
    src_addr = led_data[NRF51822_SPI_SRC_OFFSET_0];
    src_addr = (src_addr << 8) & led_data[NRF51822_SPI_SRC_OFFSET_1];
    //Extract the requested LED value
    led_value = led_data[NRF51822_SPI_PAYLOAD_OFFSET];
    
    //Set the appropriate LED depending on who send the pkt
    if(src_addr == NODE_ONE_ADDR)
    {
       if(led_value)
       {
          LED_RED_ON;
       }  
       else
       {
          LED_RED_OFF;
       }
    }
    else if(src_addr == NODE_TWO_ADDR)
    {
       if(led_value)
       {
          LED_GREEN_ON;
       }  
       else
       {
          LED_GREEN_OFF;
       }
    }  
}

void sendNodePkt(uint16_t node_addr, uint8_t led_value, msg_t *m, kernel_pid_t send_pid)
{
   transceiver_command_t trans_cmd;
   uint8_t sendPkt[sizeof(ble_radio_pkt)];
   ble_radio_pkt blePkt; 
   msg_t mResponse;

   //Fill in the BLE Pkt properly
   blePkt.msg_type = SND_PKT;
   blePkt.src_address  = 0;
   //The address to send this data to 
   blePkt.dest_address = node_addr;
   blePkt.payload[0] = led_value;

   //Fill in the packet properly
   memcpy(sendPkt, (uint8_t*)(&blePkt), sizeof(ble_radio_pkt)); 

   //Fill in the Transceiver cmd packet
   trans_cmd.transceivers = TRANSCEIVER_NRF51822BLE;
   trans_cmd.data = sendPkt;

   //Setup the message pkt
   m->type = SND_PKT;
   m->content.ptr = (char *)(&trans_cmd);

   //Send the pkt to the Transceiver
   msg_send_receive(m, &mResponse, send_pid);
}

void *rx_thread(void *arg)
{
   msg_t m_trans;

   while(1)
   {
      //Wait for an RX packet from the Transceiver
      msg_receive(&m_trans);
      //Set the LED's depending on what was received and from who
      setLED(&m_trans);
   }
}

int main(void)
{
   //Set the transceiver type to BLE
   transceiver_type_t bleTrans = TRANSCEIVER_NRF51822BLE;
   kernel_pid_t trans_pid;

   //The msg variable that will be sent to the transceiver module
   msg_t m_trans;

   //Initialize the vtimer 
   vtimer_init();

   //Initialize the BLE transceiver
   transceiver_init(bleTrans); 

   //Start the BLE transceiver
   trans_pid = transceiver_start();

   //Create a new thread that will handle receiving msg's from the BLE transceiver
   kernel_pid_t rx_pid = thread_create(rx_thread_stack, KERNEL_CONF_STACKSIZE_DEFAULT, 0, 0, rx_thread, NULL, "BLE RX THREAD"); 

   //Register this thread w/ the BLE transceiver to receive msg's
   transceiver_register(bleTrans, rx_pid);

   uint8_t led_value = 1;

   while(1)
   {
      //Send the LED Node pkt's to the Transceiver
      sendNodePkt(NODE_ONE_ADDR, led_value, &m_trans, trans_pid);
      vtimer_usleep(10000);
      sendNodePkt(NODE_TWO_ADDR, led_value ^ 1, &m_trans, trans_pid);
      
      //Alternate LED values each loop.
      led_value ^= 1;
                  
      //Sleep for 500ms and do it again
      vtimer_usleep(500000);
   }

   return 0;
}
