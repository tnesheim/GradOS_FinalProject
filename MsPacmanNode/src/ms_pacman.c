#include <stdio.h>
#include <stdlib.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "ms_pacman.h"

/*Initialize the Ms. Pacman GPIO lines to be outputs and 0xFF*/
void initMsPacmanGPIO(void)
{
   //Set pins as output
   nrf_gpio_range_cfg_output(GPIO_CTRLS_START, GPIO_CTRLS_END);
   
   //Turn all the relays off
   nrf_gpio_port_set(GPIO_RELAY_PORT, 0xFF);
}

/*Sets the GPIO pins based on the input data*/
void setMsPacmanControls(MsPacmanCtrls ms_pacman)
{
   uint8_t port_mask = 0;
   
   //Turn all the relays off
   nrf_gpio_port_set(GPIO_RELAY_PORT, 0xFF);
   
   switch(ms_pacman.arrow_direction)
   {
      case ARROW_UP:
         port_mask |= MASK_ARROW_UP;
         break;
      case ARROW_DOWN:
         port_mask |= MASK_ARROW_DOWN;
         break;
      case ARROW_LEFT:
         port_mask |= MASK_ARROW_LEFT;
         break;
      case ARROW_RIGHT:
         port_mask |= MASK_ARROW_RIGHT;
         break;
   }
   
   if(ms_pacman.button_a)
   {
      port_mask |= MASK_BUTTON_A;
   }
   
   if(ms_pacman.button_b)
   {
      port_mask |= MASK_BUTTON_B;
   }
   
   //Clear the appropriate pins to turn the relays on
   nrf_gpio_port_clear(GPIO_RELAY_PORT, port_mask);
}