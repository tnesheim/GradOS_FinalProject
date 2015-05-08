#ifndef __MSPACMAN_H__
#define __MSPACMAN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf51.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "boards.h"

#define GPIO_CTRLS_START 16
#define GPIO_CTRLS_END   21

#define MASK_BUTTON_A    0x01
#define MASK_BUTTON_B    0x02
#define MASK_ARROW_DOWN  0x04
#define MASK_ARROW_UP    0x08
#define MASK_ARROW_LEFT  0x10
#define MASK_ARROW_RIGHT 0x20

#define GPIO_RELAY_PORT 2

typedef enum
{
   ARROW_UP,
   ARROW_DOWN,
   ARROW_LEFT,
   ARROW_RIGHT,
   ARROW_NONE,
} MsPacmanArrow;

typedef struct
{
   MsPacmanArrow arrow_direction;
   bool button_a;
   bool button_b;
} MsPacmanCtrls;

/*Initialize the Ms. Pacman GPIO lines to be outputs and 0xFF*/
void initMsPacmanGPIO(void);

/*Sets the GPIO pins based on the input data*/
void setMsPacmanControls(MsPacmanCtrls ms_pacman);

#endif 