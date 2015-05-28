#include <stdio.h>
#include <stdlib.h>
#include "gps.h"

int main(void)
{
   int i;
   GPSData gps_data;
   
   //Initialize the GPS UART
   init_gps_uart();
   
   for(;;)
   {
      gps_get_data(&gps_data);
      i++;
   }
}