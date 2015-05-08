#include <stdio.h>
#include <stdlib.h>
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "ServoArmNode.h"

static uint16_t servo_claw_pulse_cc;
static uint16_t servo_claw_cc;
static uint16_t servo_wrist_pulse_cc;
static uint16_t servo_wrist_cc;

void TIMER2_IRQHandler(void)
{
   //Check to see if this interrupt occurred due to the Claw COMPARE[0] Timer 2 event
   if((NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_PULSE_CC] != 0) && 
      ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk) != 0))
   {
      //Clear the COMPARE0 event
      NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_PULSE_CC] = 0;
      
      //Increment the appropriate CC values
      NRF_TIMER2->CC[SERVO_CLAW_PULSE_CC] += servo_claw_pulse_cc;
      NRF_TIMER2->CC[SERVO_CLAW_CC] += servo_claw_pulse_cc;
   }
   //Check to see if this interrupt occurred due to the Wrist COMPARE[2] Timer 2 event
   else if((NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_PULSE_CC] != 0) && 
      ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE2_Msk) != 0))
   {
      //Clear the COMPARE2 event
      NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_PULSE_CC] = 0;
      
      //Increment the appropriate CC values
      NRF_TIMER2->CC[SERVO_WRIST_PULSE_CC] += servo_wrist_pulse_cc;
      NRF_TIMER2->CC[SERVO_WRIST_CC] += servo_wrist_pulse_cc;
   }
   //Check to see if this interrupt occurred due to the Wrist COMPARE[2] Timer 2 event
   /*else if((NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_CC] != 0) && 
      ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE1_Msk) != 0))
   {
      //Clear the COMPARE2 event
      NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_CC] = 0;
      
      //Increment the appropriate CC values
      NRF_TIMER2->CC[SERVO_CLAW_CC] += servo_claw_cc;
   }
   //Check to see if this interrupt occurred due to the Wrist COMPARE[2] Timer 2 event
   else if((NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_CC] != 0) && 
      ((NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE3_Msk) != 0))
   {
      //Clear the COMPARE2 event
      NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_CC] = 0;
      
      //Increment the appropriate CC values
      NRF_TIMER2->CC[SERVO_WRIST_CC] += servo_wrist_cc;
   } */
}

void TIMER1_IRQHandler(void)
{
   //Check to see if this interrupt occurred due to the COMPARE0 Timer 2 event
   if((NRF_TIMER1->EVENTS_COMPARE[SERVO_ARM_PULSE_CC] != 0) && 
      ((NRF_TIMER1->INTENSET & TIMER_INTENSET_COMPARE0_Msk) != 0))
   {
      //Clear the COMPARE0 event
      NRF_TIMER1->EVENTS_COMPARE[SERVO_ARM_PULSE_CC] = 0;
      
      //Clear the timer so that another COMPARE0 event will happen in 20ms + arm_width for the Servos
      NRF_TIMER1->TASKS_CLEAR = 1;
   }
}

/*Initializes the servos and puts them in their neutral positions*/
void initServos()
{
   //Initialize the Servo GPIO pins and tasks
   gpiote_init();
   
   //Configure the PPI for the Servo GPIO tasks and their corresponding Timer 2 events
   ppi_init();
   
   //Initialize Timer 2 and set the initial servo positions through PWM pulses
   servo_timers_init();
   
   // Enable interrupt on Timer 2 and Timer 1.
   NVIC_EnableIRQ(TIMER2_IRQn);
   NVIC_EnableIRQ(TIMER1_IRQn);
   __enable_irq();

   // Start the timer.
   NRF_TIMER2->TASKS_START = 1;
   NRF_TIMER1->TASKS_START = 1;
}

/*Set the position of the servo*/
void setServoPosition(uint8_t servo_num, uint16_t servo_pos)
{
   if(servo_num == SERVO_CLAW_GPIO)
   {
      servo_claw_pulse_cc = SERVO_PULSE_CYCLE_20_MS + servo_pos;
      servo_claw_cc       = servo_pos;
   }
   else if(servo_num == SERVO_WRIST_GPIO)
   {
      servo_wrist_pulse_cc = SERVO_PULSE_CYCLE_20_MS + servo_pos;
      servo_wrist_cc       = servo_pos;
   }
   else if(servo_num == SERVO_ARM_GPIO)
   {
      NRF_TIMER1->CC[SERVO_ARM_PULSE_CC] = SERVO_PULSE_CYCLE_20_MS + servo_pos; 
      NRF_TIMER1->CC[SERVO_ARM_CC]       = servo_pos;
   }
}

/*Initialize Timer 2 and Timer 1 for PWM Servo control*/
void servo_timers_init(void)
{
    // Start 16 MHz crystal oscillator .
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    // Wait for the external oscillator to start up.
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) 
    {
        //Do nothing.
    }

    ////!!! Initialize Timer 2 for Claw & Wrist Servos !!!////
    
    NRF_TIMER2->MODE      = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->BITMODE   = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER2->PRESCALER = TIMER_PRESCALER;

    // Clears the timer, sets it to 0.
    NRF_TIMER2->TASKS_CLEAR = 1;
    
    //Set the value of the Servos pulse cycle to be a constant 20ms
    servo_claw_pulse_cc = SERVO_PULSE_CYCLE_20_MS + SERVO_NEUTRAL_POSITION_1_5_MS;
    NRF_TIMER2->CC[SERVO_CLAW_PULSE_CC]  = servo_claw_pulse_cc;
    servo_wrist_pulse_cc = SERVO_PULSE_CYCLE_20_MS + SERVO_NEUTRAL_POSITION_1_5_MS;
    NRF_TIMER2->CC[SERVO_WRIST_PULSE_CC] = servo_wrist_pulse_cc;
    
    //Set the Servos initial PWM values to 1.5ms, aka 90 degrees
    servo_claw_cc = SERVO_NEUTRAL_POSITION_1_5_MS;
    NRF_TIMER2->CC[SERVO_CLAW_CC]  = servo_claw_cc;
    servo_wrist_cc = SERVO_NEUTRAL_POSITION_1_5_MS;
    NRF_TIMER2->CC[SERVO_WRIST_CC] = servo_wrist_cc;
    
    //Enable Timer 2 interrupts for Compare 0 and Compare 2
    NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
    NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
    NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE2_Enabled << TIMER_INTENSET_COMPARE2_Pos);
    NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE3_Enabled << TIMER_INTENSET_COMPARE3_Pos);
    
    ////!!! Initialize Timer 1 for Arm Servo !!!////
    
    NRF_TIMER1->MODE      = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE   = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER = TIMER_PRESCALER;

    // Clears the timer, sets it to 0.
    NRF_TIMER1->TASKS_CLEAR = 1;

    //Set the value of the Servos pulse cycle to be a constant 20ms
    NRF_TIMER1->CC[SERVO_ARM_PULSE_CC]  = SERVO_PULSE_CYCLE_20_MS + SERVO_NEUTRAL_POSITION_1_5_MS;
    
    //Set the Servos initial PWM values to 1.5ms, aka 90 degrees
    NRF_TIMER1->CC[SERVO_ARM_CC]  = SERVO_NEUTRAL_POSITION_1_5_MS;
    
    //Enable Timer 1 interrupts for Compare 0
    NRF_TIMER1->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
}

/*Initialize the Servo GPIO control lines to be output and to toggle when their task is called.*/
void gpiote_init(void)
{
   //Set the Servo GPIO pins to be outputs
   nrf_gpio_cfg_output(SERVO_CLAW_GPIO);
   nrf_gpio_cfg_output(SERVO_WRIST_GPIO);
   nrf_gpio_cfg_output(SERVO_ARM_GPIO);
   
   nrf_gpio_port_clear(NRF_GPIO_PORT_SELECT_PORT0, 0xFF);
   
   //Configure each Servo GPIO pin to be attached to a task that toggles its value
   nrf_gpiote_task_config(SERVO_CLAW_GPIO_TASK, SERVO_CLAW_GPIO, \
                           NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
   nrf_gpiote_task_config(SERVO_WRIST_GPIO_TASK, SERVO_WRIST_GPIO, \
                           NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
   nrf_gpiote_task_config(SERVO_ARM_GPIO_TASK, SERVO_ARM_GPIO, \
                           NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
}

/*Initializes the Programmable Peripheral Interconnect to toggle the Servo PWMs upon timer compare matches*/
void ppi_init(void)
{
    // Configure PPI channel 0-2 to toggle each Servo on every TIMER2/TIMER1 COMPARE[0] match, aka every 20ms.
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_PULSE_CC];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_CLAW_GPIO_TASK];
    NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_PULSE_CC];
    NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_WRIST_GPIO_TASK];
    //Timer 1 Compare for the Arm Servo
    NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[SERVO_ARM_PULSE_CC];
    NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_ARM_GPIO_TASK];

    // Configure PPI channel 3 to toggle the Claw Servo on Timer 2 COMPARE[1] matches
    NRF_PPI->CH[3].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[SERVO_CLAW_CC];
    NRF_PPI->CH[3].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_CLAW_GPIO_TASK];
    
    // Configure PPI channel 4 to toggle the Wrist Servo on Timer 2 COMPARE[3] matches
    NRF_PPI->CH[4].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[SERVO_WRIST_CC];
    NRF_PPI->CH[4].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_WRIST_GPIO_TASK];
   
    // Configure PPI channel 5 to toggle the Arm Servo on Timer 1 COMPARE[1] matches
    NRF_PPI->CH[5].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[SERVO_ARM_CC];
    NRF_PPI->CH[5].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERVO_ARM_GPIO_TASK];
   
    // Enable PPI channels 0-5.
    NRF_PPI->CHEN =   (PPI_CHEN_CH0_Enabled << PPI_CHEN_CH0_Pos)
                    | (PPI_CHEN_CH1_Enabled << PPI_CHEN_CH1_Pos)
                    | (PPI_CHEN_CH2_Enabled << PPI_CHEN_CH2_Pos)
                    | (PPI_CHEN_CH3_Enabled << PPI_CHEN_CH3_Pos)
                    | (PPI_CHEN_CH4_Enabled << PPI_CHEN_CH4_Pos)
                    | (PPI_CHEN_CH5_Enabled << PPI_CHEN_CH5_Pos);
}