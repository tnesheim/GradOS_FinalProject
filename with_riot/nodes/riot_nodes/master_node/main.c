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

int main(void)
{
   //Set the transceiver type to BLE
   transceiver_type_t bleTrans = TRANSCEIVER_NRF51822BLE;
   kernel_pid_t trans_pid;

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

   LED_RED_ON;

   while(1)
   {
   }

   return 0;
}
