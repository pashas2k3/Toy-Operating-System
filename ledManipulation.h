/*
 * ledManipulation.h
 *
 *  Created on: Oct 3, 2015
 *      Author: Vikram
 */

#ifndef LEDMANIPULATION_H_
#define LEDMANIPULATION_H_

#include <stdio.h>
#include <stdint.h>

#define LED_ORANGE_PORTA_BIT 11
#define LED_YELLOW_PORTA_BIT 28
#define LED_GREEN_PORTA_BIT 29
#define LED_BLUE_PORTA_BIT 10

#define PUSHBUTTON_SW1_PORTD_BIT 0
#define PUSHBUTTON_SW2_PORTE_BIT 26

#ifndef PORT_PCR_MUX_ANALOG
#define PORT_PCR_MUX_ANALOG 0
#endif
#ifndef PORT_PCR_MUX_GPIO
#define PORT_PCR_MUX_GPIO 1
#endif

/****************DELAY ROUTINES***************/
void delay(unsigned long int limit);

/* Assembler routine to delay for user specified interval
 * 
 * With 120 MHz clock, each loop is 4 cycles * (1/120,000,000) seconds cycle
 * time.  So, each loop is .0000000333 seconds = 33.333 nanoseconds. */
void asmDelay(unsigned long int limit);


/****************PUSH BUTTON ROUTINES***************/
/* Routine to initialize both of the pushbuttons */
/* Note: This procedure *does* enable the appropriate port clocks */
void pushbuttonInitAll(void);

void pushButtonInitClock(void);

/* Routine to configure pushbutton SW1 */
/* Note: This procedure does not enable the port clock */
void SW1Config(void);
/* Routine to configure pushbutton SW2 */
/* Note: This procedure does not enable the port clock */
void SW2Config(void);

/* Routine to read state of pushbutton SW1 */
int SW1In(void);
/* Routine to read state of pushbutton SW2 */
int SW2In(void);

/****************LED ROUTINES***************/
/* Routine to initialize all of the LEDs */
/* Note: This procedure *does* enable the appropriate port clock */
void ledInitAll(void);

void ledInitClock(void);

/* Routine to configure the orange LED */
/* Note: This procedure does not enable the port clock */
void ledOrangeConfig(void);
/* Routine to configure the yellow LED */
/* Note: This procedure does not enable the port clock */
void ledYellowConfig(void);
/* Routine to configure the green LED */
/* Note: This procedure does not enable the port clock */
void ledGreenConfig(void);
/* Routine to configure the blue LED */
/* Note: This procedure does not enable the port clock */
void ledBlueConfig(void);

/* Routine to turn off the orange LED */
void ledOrangeOff(void);
/* Routine to turn off the yellow LED */
void ledYellowOff(void);
/* Routine to turn off the green LED */
void ledGreenOff(void);
/* Routine to turn off the blue LED */
void ledBlueOff(void);

/* Routine to turn on the orange LED */
void ledOrangeOn(void);
/* Routine to turn on the yellow LED */
void ledYellowOn(void);
/* Routine to turn on the green LED */
void ledGreenOn(void);
/* Routine to turn on the blue LED */
void ledBlueOn(void);

/**********************************/
void TSI_Init(void);
void TSI_Calibrate(void);
int electrode_in(int electrodeNumber);
#endif /* LEDMANIPULATION_H_ */
