#ifndef __GPS_H__
#define __GPS_H__

#include <stdio.h>
#include <stdlib.h>
#include "simple_uart.h"

#define GPS_TX 3
#define GPS_RX 4

#define GPS_DATA_FIELD_LEN 15
#define GPS_START_PKT '$'
#define GPS_DATA_SEPARATOR ','
#define GPS_HDR_GGA "GPGGA"

//Struct that contains the ASCII GPS Data
typedef struct
{
   uint8_t gps_header[GPS_DATA_FIELD_LEN];
   uint8_t utc_time[GPS_DATA_FIELD_LEN];
   uint8_t latitude[GPS_DATA_FIELD_LEN];
   uint8_t north_south;
   uint8_t longitude[GPS_DATA_FIELD_LEN];
   uint8_t east_west;
   uint8_t position_fix;
} GPSData;

/*Initialize the GPS UART connection*/
void init_gps_uart(void);

/*Polls the GPS UART until a new packet arrives, then fills in the appropriate amount of data*/
void gps_get_data(GPSData *gps_data);

/*Read in a single GPS field*/
void gps_get_field(uint8_t *gps_field_buf, uint8_t buf_len);

#endif
