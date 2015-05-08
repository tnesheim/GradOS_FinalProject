#include <stdio.h>
#include <stdlib.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "ms_pacman.h"

int main(void)
{
   int i;
   MsPacmanCtrls ms_pacman;
   
   initMsPacmanGPIO();
   
   ms_pacman.arrow_direction = ARROW_UP;
   ms_pacman.button_b = true;
   ms_pacman.button_a = false;
   
   setMsPacmanControls(ms_pacman);
   
   for(;;)
   {
      for(i = 0; i < 5; i++)
      {
         ms_pacman.arrow_direction = i;
         setMsPacmanControls(ms_pacman);
         nrf_delay_us(1000000);
      }
   }
}