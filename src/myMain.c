/*
 * myMain.c
 *
 *  Created on: Oct 3, 2015
 *      Author: Vikram
 */

#include "derivative.h" /*Do we need this?*/
#include "ledManipulation.h"

#define FALSE 0
#define TRUE 1

inline void ledOffAll(void)
{
	ledOrangeOff();
	ledYellowOff();
	ledBlueOff();
	ledGreenOff();
}

int main(){
	ledInitAll();
	pushbuttonInitAll();
	
	uint8_t counter = 0;
	uint8_t buttonPressDetected = FALSE, buttonPressed = TRUE;
	while(TRUE)
	{
		if(buttonPressed){
			ledOffAll();
			switch(counter & 3){
			case 0:
				ledOrangeOn(); break;		
			case 1:
				ledYellowOn(); break;
			case 2:
				ledGreenOn();break;
			case 3:
				ledBlueOn();break;
			}
			buttonPressed = FALSE;
		}
		if(SW1In())/*Check for button press*/
		{
			/*To avoid repeated counter increments 
			 * unless released once */
			if(!buttonPressDetected){
				/*A small amount of delay to skip over bouncing contact*/
				asmDelay(100);
				/*increment the counter*/
				++counter;
				buttonPressDetected = TRUE;
			}
			/**/
		} else{
			/*To make sure the change is enforced after the button is released*/
			if(buttonPressDetected){
				buttonPressed = TRUE;
				buttonPressDetected = FALSE;
			}
		}

		
	}	//end while loop
	return 0;
}
