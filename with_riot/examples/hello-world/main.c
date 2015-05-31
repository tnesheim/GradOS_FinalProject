/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include "board.h"
#include "softdevice_handler.h"

int main(void)
{
    LED_RED_ON;

    puts("Hello World!");
    uint8_t a;
    uint16_t b;
    char ret = sd_ble_evt_get(&a,&b);
    printf("output %i\n",ret);

    return 0;
}