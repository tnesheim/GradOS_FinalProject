#include <stdio.h>
#include <stdlib.h>
#include "twi_master.h"
#include "nunchuck.h"
#include "simple_uart.h"

#define TX_PIN 8
#define RX_PIN 9

int main(void)
{
   bool success;
   NunchuckData nunData;
   
   //Initialize the Nunchuck
   success = initNunchuck();
   
   for(;;)
   {
      //Get data from the Nunchuck periodically
      success = readNunchuckData(&nunData);
   }
}