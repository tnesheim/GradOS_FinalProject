#include <stdio.h>
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
#include "ms_pacman.h"
#include "nunchuck.h"

#define NODE_ONE_ADDR 0xD92F
#define NODE_TWO_ADDR 0xD35F
#define MS_PACMAN_ADDR 0xD92F

void createTransceiverRcvMsg(msg_t *m)
{
   m->type = RCV_PKT_NRF51822BLE;
   m->content.value = 0; 
}

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

MsPacmanCtrls translateToMsPacman(msg_t *m)
{
    uint8_t nunchuck_data[NRF51822_MAX_DATA_LENGTH];
    radio_packet_t *trans_buf;

    //Copy the data from the transceiver
    trans_buf = (radio_packet_t *)(m->content.ptr);
    memcpy(nunchuck_data, trans_buf->data, NRF51822_MAX_DATA_LENGTH);

    //Tell the Transceiver that we are done with the buffer
    trans_buf->processing = 0;

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

void sendMsPacmanPkt(MsPacmanCtrls ms_pacman, msg_t *m, kernel_pid_t send_pid)
{
   transceiver_command_t trans_cmd;
   uint8_t sendPkt[sizeof(ble_radio_pkt)];
   ble_radio_pkt blePkt; 
   msg_t mResponse;

   //Fill in the BLE Pkt properly
   blePkt.msg_type = SND_PKT;
   blePkt.src_address  = 0;
   //The address to send this data to, aka the Ms. Pacman node 
   blePkt.dest_address = MS_PACMAN_ADDR;
   blePkt.payload[BLE_MS_PACMAN_OFFSET_ARROW] = ms_pacman.arrow_direction;
   blePkt.payload[BLE_MS_PACMAN_OFFSET_BUTTON_A] = ms_pacman.button_a;
   blePkt.payload[BLE_MS_PACMAN_OFFSET_BUTTON_B] = ms_pacman.button_b;

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

int main(void)
{
   MsPacmanCtrls ms_pacman;

   //Set the transceiver type to BLE
   transceiver_type_t bleTrans = TRANSCEIVER_NRF51822BLE;
   kernel_pid_t trans_pid;

   //The msg variable that will be sent to the transceiver module
   msg_t m_trans;

   //Get the PID of the main thread
   kernel_pid_t mthread_pid = thread_getpid();

   //Initialize the vtimer 
   vtimer_init();

   //Initialize the BLE transceiver
   transceiver_init(bleTrans); 

   //Start the BLE transceiver
   trans_pid = transceiver_start();

   //Register this thread w/ the BLE transceiver to receive msg's
   transceiver_register(bleTrans, mthread_pid);

   uint8_t led_value = 1;

   while(1)
   {
      //Request to RCV a packet from the Transceiver
      createTransceiverRcvMsg(&m_trans);
      msg_send(&m_trans, trans_pid);
      //Now wait for a response
      msg_receive(&m_trans);
      //Set the LED's depending on what was received and from who
      setLED(&m_trans);
      //Send the LED Node pkt's to the Transceiver
      sendNodePkt(NODE_ONE_ADDR, led_value, &m_trans, trans_pid);
      sendNodePkt(NODE_TWO_ADDR, led_value ^ 1, &m_trans, trans_pid);
      led_value ^= 1;
                  
      //Sleep for 500ms and do it again
      vtimer_usleep(500000);
      
   }

   return 0;
}
