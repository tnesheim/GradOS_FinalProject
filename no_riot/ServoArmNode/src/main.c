#include <stdio.h>
#include <stdlib.h>
#include "ServoArmNode.h"
#include "nrf_delay.h"
#include "boards.h"

void nrf_delay_ms(uint32_t delay)
{
   int i;
   
   for(i = 0; i < delay; i++)
   {
      nrf_delay_us(1000);
   }
}

int main(void)
{
   //Initialize the servos and start the timers going
   initServos();
   
   //setServoPosition(SERVO_CLAW_GPIO, 2500);
   //setServoPosition(SERVO_WRIST_GPIO, 3000);
   //setServoPosition(SERVO_ARM_GPIO, 2000);
   setServoPosition(SERVO_CLAW_GPIO, 2000);
   
   for(;;)
   {
   }
}