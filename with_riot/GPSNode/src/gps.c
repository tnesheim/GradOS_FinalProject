#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_uart.h"
#include "gps.h"

/*Initialize the GPS UART connection*/
void init_gps_uart()
{
   //Initialize the UART
   simple_uart_config(0, GPS_TX, 0, GPS_RX, false);
}

/*Polls the GPS UART until a new packet arrives, then fills in the appropriate amount of data*/
void gps_get_data(GPSData *gps_data)
{
   uint8_t gps_val;
   
   //Keep looping until we are at the GGA packet
   do
   {
      //Synchronize UART to get data at the beginning of a packet
      do
      {
         gps_val = simple_uart_get();
      } while(gps_val != GPS_START_PKT);
      
      //Get the header
      gps_get_field(gps_data->gps_header, GPS_DATA_FIELD_LEN);
      
   } while(strcmp(gps_data->gps_header, GPS_HDR_GGA) != 0);
   
   //Fill in the UTC time parameter
   gps_get_field(gps_data->utc_time, GPS_DATA_FIELD_LEN);
   
   //Fill in the Latitude parameter
   gps_get_field(gps_data->latitude, GPS_DATA_FIELD_LEN);
   
   //Fill in the North/South parameter
   gps_get_field(&(gps_data->north_south), 1);
   
   //Fill in the Longitude parameter
   gps_get_field(gps_data->longitude, GPS_DATA_FIELD_LEN);
   
   //Fill in the East/West parameter
   gps_get_field(&(gps_data->east_west), 1);
   
   //Fill in the Position fix parameter
   gps_get_field(&(gps_data->position_fix), 1);
}

/*Read in a single ASCII GPS field. Assuming we are at the start of a field.*/
void gps_get_field(uint8_t *gps_field_buf, uint8_t buf_len)
{
   int i;
   uint8_t gps_val;
   
   //If we are looking for a single char field, get that and the separator
   if(buf_len == 1)
   {
      //Get the field value
      gps_field_buf[0] = simple_uart_get();
      
      //Get the separator and discard it
      simple_uart_get();
      
      return;
   }
   
   for(i = 0; i < buf_len; i++)
   {
      //Get the next available byte from the GPS
      gps_val = simple_uart_get();
      
      //End the field extraction when the data separator is encountered
      if(gps_val == GPS_DATA_SEPARATOR)
      {
         //Null terminate the ASCII string
         gps_field_buf[i] = '\0';
         
         break;
      }
      
      //Store the received value in the buffer
      gps_field_buf[i] = gps_val;
      
   }
}
