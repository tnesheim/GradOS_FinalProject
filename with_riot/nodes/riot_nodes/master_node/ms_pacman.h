#ifndef __MSPACMAN_H__
#define __MSPACMAN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ms_pacman_service.h"

typedef enum
{
   ARROW_UP,
   ARROW_DOWN,
   ARROW_LEFT,
   ARROW_RIGHT,
   ARROW_NONE,
} MsPacmanArrow;

typedef struct MsPacmanCtrls
{
   MsPacmanArrow arrow_direction;
   bool button_a;
   bool button_b;
} MsPacmanCtrls;

#define GPIO_CTRLS_START 16
#define GPIO_CTRLS_END   21

#define MASK_BUTTON_A    0x01
#define MASK_BUTTON_B    0x02
#define MASK_ARROW_DOWN  0x04
#define MASK_ARROW_UP    0x08
#define MASK_ARROW_LEFT  0x10
#define MASK_ARROW_RIGHT 0x20

#define GPIO_RELAY_PORT 2

#endif 
