#ifndef __SERVO_ARM_NODE_H__
#define __SERVO_ARM_NODE_H__

#include <stdio.h>
#include <stdlib.h>
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "boards.h"

//Servo constants
#define SERVO_CLAW_GPIO  8
#define SERVO_WRIST_GPIO 9
#define SERVO_ARM_GPIO   12

#define SERVO_PULSE_CYCLE_20_MS 20000     //20ms
#define SERVO_NEUTRAL_POSITION_1_5_MS 1500 //1.5ms

#define SERVO_CLAW_PULSE_CC  0
#define SERVO_CLAW_CC        1
#define SERVO_WRIST_PULSE_CC 2
#define SERVO_WRIST_CC       3

//The arm will be on Timer 1
#define SERVO_ARM_PULSE_CC 0
#define SERVO_ARM_CC       1

#define SERVO_CLAW_GPIO_TASK  0
#define SERVO_WRIST_GPIO_TASK 1
#define SERVO_ARM_GPIO_TASK   2

//Timer constants
#define TIMER_PRESCALER 4 //Prescale timer to a 1Mhz frequency, aka 1us tick

/*Initializes the servos and puts them in their neutral positions*/
void initServos();

/*Set the position of the servo*/
void setServoPosition(uint8_t servo_num, uint16_t servo_pos);

/*Initialize Timer 2 and Timer 1 for PWM Servo control*/
void servo_timers_init(void);

/*Initialize the Servo GPIO control lines to be output and to toggle when their task is called.*/
void gpiote_init(void);

/*Initializes the Programmable Peripheral Interconnect to toggle the Servo PWMs upon timer compare matches*/
void ppi_init(void);

#endif