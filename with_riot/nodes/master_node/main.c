#include <stdio.h>
#include "board.h"
#include "kernel.h"
#include "periph/spi.h"
#include "transceiver.h"
#include "vtimer.h"

int main(void)
{
   //Set the transceiver type to BLE
   transceiver_type_t bleTrans = TRANSCEIVER_NRF51822BLE;
   kernel_pid_t trans_pid;

   //Get the PID of the main thread
   kernel_pid_t mthread_pid = thread_getpid();

   //Initialize the vtimer and wait for a bit to synchronize the uC's
   vtimer_init();
   vtimer_usleep(500000); //500ms

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
