/*
 * ledManipulation.c

 *
 *  Created on: Oct 3, 2015
 *      Author: Vikram
 */


#include "derivative.h"
#include "ledManipulation.h"
#include "utility.h"
/****************DELAY ROUTINES***************/
/**
 * Routine to delay for user specified interval
 */ 
void delay(unsigned long int limit) {
  unsigned long int i;
  for(i = 0; i < limit; i++) {
  }
}

/**
 * Assembly routine to delay for user specified interval
 * 
 * With 120 MHz clock, each loop is 4 cycles * (1/120,000,000) seconds cycle
 * time.  So, each loop is .0000000333 seconds = 33.333 nanoseconds.
 */ 
void asmDelay(unsigned long int limit);
__asm("\n\
	.global	asmDelay\n\
asmDelay:\n\
	adds	r0,r0,#-1\n\
	bne	asmDelay\n\
	bx	lr\n\
      ");

/*******************PUSH BUTTON ROUTINES****************************************/

void pushButtonInitClock(void)
{
 /* Enable the clocks for PORTD & PORTE */
  SIM_SCGC5 |= (SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK);
}

void pushbuttonInitAll(){
  pushButtonInitClock();

  SW1Config();
  SW2Config();
}

/**
 * Routine to configure pushbutton SW1
 * 
 * Note: This procedure does not enable the port clock
*/
void SW1Config(void) {
  /* Configure bit PUSHBUTTON_SW1_PORTD_BIT of PORTD using the Pin Control
   * Register (PORTD_PCR) to be a GPIO pin.  This sets the MUX field (Pin Mux
   * Control) to GPIO mode (Alternative 1).  Also, by setting the PE bit,
   * enable the internal pull-up or pull-down resistor.  And, by setting the
   * PS bit, enable the internal pull-up resistor -- not the pull-down
   * resistor.  (See 11.4.1 on page 309 of the K70 Sub-Family Reference
   * Manual, Rev. 2, Dec 2011) */
  PORT_PCR_REG(PORTD_BASE_PTR, PUSHBUTTON_SW1_PORTD_BIT) =
    PORT_PCR_MUX(PORT_PCR_MUX_GPIO) |
    PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

  /* By not setting the Port Data Direction Register (GPIOx_PDDR) to be a GPIO
   * output, we have configured bit PUSHBUTTON_SW1_PORTD_BIT of PORTD to be a
   * GPIO input.  (See 60.2.6 on page 2155 of the K70 Sub-Family Reference
   * Manual, Rev. 2, Dec 2011) */
}

/**
 * Routine to configure pushbutton SW2
 * 
 * Note: This procedure does not enable the port clock
*/
void SW2Config(void) {
  /* Configure bit PUSHBUTTON_SW2_PORTE_BIT of PORTE to be a GPIO pin with an
   * internal pull-up resistor. */
  PORT_PCR_REG(PORTE_BASE_PTR, PUSHBUTTON_SW2_PORTE_BIT) =
    PORT_PCR_MUX(PORT_PCR_MUX_GPIO) |
    PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

  /* Configure bit PUSHBUTTON_SW2_PORTE_BIT of PORTE to be a GPIO input. */
}

/**
 * Routine to read state of pushbutton SW1
*/
int SW1In(void) {
  /* Returns 1 when pushbutton SW1 is depressed and 0 otherwise */
  int pushbuttonState;
	
  /* Read the state of bit PUSHBUTTON_SW1_PORTD_BIT of PORTD using the Port Data
   * Input Register (GPIOx_PDIR).  (See 60.2.5 on page 2155 of the K70
   * Sub-Family Reference Manual, Rev. 2, Dec 2011) */
  pushbuttonState = PTD_BASE_PTR->PDIR & (1 << PUSHBUTTON_SW1_PORTD_BIT);
  return !pushbuttonState;
}

/**
 * Routine to read state of pushbutton SW2
*/
int SW2In(void) {
  /* Returns 1 when pushbutton SW2 is depressed and 0 otherwise */
  int pushbuttonState;
	
  /* Read the state of bit PUSHBUTTON_SW2_PORTE_BIT of PORTE */
  pushbuttonState = PTE_BASE_PTR->PDIR & (1 << PUSHBUTTON_SW2_PORTE_BIT);
  return !pushbuttonState;
}

/******************LED ROUTINES****************************************/
void ledInitClock(void)
{
  /* Enable the clock for PORTA using the SIM_SCGC5 register (System Clock Gating
   * Control Register 5) (See 12.2.13 on page 342 of the K70 Sub-Family Reference
   * Manual, Rev. 2, Dec 2011) */
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
}

void ledInitAll(void) {
	
  ledInitClock();
  /* Turn off the LEDs */
  ledOrangeOff();
  ledYellowOff();
  ledGreenOff();
  ledBlueOff();

  /* Configure the LED ports */
  ledOrangeConfig();
  ledYellowConfig();
  ledGreenConfig();
  ledBlueConfig();
}

/**
 * Routine to configure the orange LED
 * 
 * Note: This procedure does not enable the port clock
 */ 
void ledOrangeConfig(void) {
	/* Configure bit LED_ORANGE_PORTA_BIT of PORTA using the Pin Control Register
	 * (PORTA_PCR) to be a GPIO pin.  This sets the MUX field (Pin Mux Control) to
	 * GPIO mode (Alternative 1).  (See 11.4.1 on page 309 of the K70 Sub-Family
	 * Reference Manual, Rev. 2, Dec 2011) */
	PORT_PCR_REG(PORTA_BASE_PTR, LED_ORANGE_PORTA_BIT) =
			PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
	/* Above is equivalent to:
	PORTA_PCR11 = PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
	 * but uses my #define for the appropriate bit number.
	 */
	
	/* Configure bit LED_ORANGE_PORTA_BIT of PORTA using the Port Data Direction
	 * Register (GPIOx_PDDR) to be a GPIO output.  (See 60.2.6 on page 2155 of the
	 * K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
	PTA_BASE_PTR->PDDR |= 1<<LED_ORANGE_PORTA_BIT;
}

/**
 * Routine to configure the yellow LED
 * 
 * Note: This procedure does not enable the port clock
 */ 
void ledYellowConfig(void) {
	/* Configure bit LED_YELLOW_PORTA_BIT of PORTA to be a GPIO pin. */
	PORT_PCR_REG(PORTA_BASE_PTR, LED_YELLOW_PORTA_BIT) =
			PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
	/* Configure bit LED_YELLOW_PORTA_BIT of PORTA to be a GPIO output. */
	PTA_BASE_PTR->PDDR |= 1<<LED_YELLOW_PORTA_BIT;
}

/**
 * Routine to configure the green LED
 * 
 * Note: This procedure does not enable the port clock
 */ 
void ledGreenConfig(void) {
	/* Configure bit LED_GREEN_PORTA_BIT of PORTA to be a GPIO pin. */
	PORT_PCR_REG(PORTA_BASE_PTR, LED_GREEN_PORTA_BIT) =
			PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
	/* Configure bit LED_GREEN_PORTA_BIT of PORTA to be a GPIO output. */
	PTA_BASE_PTR->PDDR |= 1<<LED_GREEN_PORTA_BIT;
}

/**
 * Routine to configure the blue LED
 * 
 * Note: This procedure does not enable the port clock
 */ 
void ledBlueConfig(void) {
	/* Configure bit LED_BLUE_PORTA_BIT of PORTA to be a GPIO pin. */
	PORT_PCR_REG(PORTA_BASE_PTR, LED_BLUE_PORTA_BIT) =
			PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
	/* Configure bit LED_BLUE_PORTA_BIT of PORTA to be a GPIO output. */
	PTA_BASE_PTR->PDDR |= 1<<LED_BLUE_PORTA_BIT;
}

/**
 * Routine to turn off the orange LED
 */ 
void ledOrangeOff(void) {
	/* Turn off the orange LED by setting bit LED_ORANGE_PORTA_BIT of PORTA using
	 * the Port Set Output Register (GPIOx_PSOR).  (See 60.2.2 on page 2153 of the
	 * K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
	PTA_BASE_PTR->PSOR = 1<<LED_ORANGE_PORTA_BIT;
}

/**
 * Routine to turn off the yellow LED
 */ 
void ledYellowOff(void) {
	/* Turn off the yellow LED */
	PTA_BASE_PTR->PSOR = 1<<LED_YELLOW_PORTA_BIT;
}

/**
 * Routine to turn off the green LED
 */ 
void ledGreenOff(void) {
	/* Turn off the green LED */
	PTA_BASE_PTR->PSOR = 1<<LED_GREEN_PORTA_BIT;
}

/**
 * Routine to turn off the blue LED
 */ 
void ledBlueOff(void) {
	/* Turn off the blue LED */
	PTA_BASE_PTR->PSOR = 1<<LED_BLUE_PORTA_BIT;
}

/**
 * Routine to turn on the orange LED
 */ 
void ledOrangeOn(void) {
	/* Turn on the orange LED by clearing bit LED_ORANGE_PORTA_BIT of PORTA using
	 * the Port Clear Output Register (GPIOx_PCOR).  (See 60.2.3 on page 2153 of the
	 * K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
	PTA_BASE_PTR->PCOR = 1<<LED_ORANGE_PORTA_BIT;
}

/**
 * Routine to turn on the yellow LED
 */ 
void ledYellowOn(void) {
	/* Turn on the yellow LED */
	PTA_BASE_PTR->PCOR = 1<<LED_YELLOW_PORTA_BIT;
}

/**
 * Routine to turn on the green LED
 */ 
void ledGreenOn(void) {
	/* Turn on the green LED */
	PTA_BASE_PTR->PCOR = 1<<LED_GREEN_PORTA_BIT;
}

/**
 * Routine to turn on the blue LED
 */ 
void ledBlueOn(void) {
	/* Turn on the blue LED */
	PTA_BASE_PTR->PCOR = 1<<LED_BLUE_PORTA_BIT;
}
/*******************TOUCH SENSOR CODE************************************/
#define PORT_PCR_MUX_ANALOG 0
#define PORT_PCR_MUX_GPIO 1

#define ELECTRODE_COUNT 4
#define THRESHOLD_OFFSET 0x200

/* Note below that the counterRegister field is declared to be a pointer
 * to an unsigned 16-bit value that is the counter register for the
 * appropriate channel of TSI0.  Further note that TSI0_CNTR5, for
 * example, is a 32-bit register that contains two 16-bit counters --
 * one for channel 4 in the low half and one for channel 5 in the high
 * half.  So, once &TSI0_CNTR5 is cast to be a pointer to a 16-bit
 * unsigned int, it is then a pointer to the 16-bit counter for
 * channel 4 of TSI0.  That pointer then needs to be incremented by
 * one so that it points to the 16-bit counter for channel 5 of TSI0.
 * This same technique is used for all the other counterRegister
 * field values as well. */
struct electrodeHW {
	int channel;
	uint32_t mask;
	uint16_t threshold;
	uint16_t *counterRegister;
} electrodeHW[ELECTRODE_COUNT] =
	{{5, TSI_PEN_PEN5_MASK, 0, (uint16_t *)&TSI0_CNTR5+1},	/* E1 */
	 {8, TSI_PEN_PEN8_MASK, 0, (uint16_t *)&TSI0_CNTR9},	/* E2 */
	 {7, TSI_PEN_PEN7_MASK, 0, (uint16_t *)&TSI0_CNTR7+1},	/* E3 */
	 {9, TSI_PEN_PEN9_MASK, 0, (uint16_t *)&TSI0_CNTR9+1}};	/* E4 */

/* Initialize the capacitive touch sensors */
void TSI_Init(void) {
    SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_TSI_MASK;
    									// Turn on the clock to ports A & B and
    									//		to the TSI module
    PORTA_PCR4 = PORT_PCR_MUX(0);		// Enable port A, bit 4 as TSI0_CH5
    PORTB_PCR3 = PORT_PCR_MUX(0);		// Enable port B, bit 3 as TSI0_CH8
    PORTB_PCR2 = PORT_PCR_MUX(0);		// Enable port B, bit 2 as TSI0_CH7
    PORTB_PCR16 = PORT_PCR_MUX(0);		// Enable port B, bit 16 as TSI0_CH9

    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    TSI0_GENCS |= TSI_GENCS_NSCN(10) |	// Set number of consecutive scans per electrode to 11
    		TSI_GENCS_PS(4) |			// Set electrode oscillator prescaler to divide by 16
    		TSI_GENCS_STPE_MASK |		// Keep TSI running when MCU goes into low power mode
    		TSI_GENCS_LPSCNITV(7);		// Low power mode scan interval is set to 50 msec
    TSI0_SCANC |= (TSI_SCANC_EXTCHRG(8) |	// Set ext oscillator charge current to 18 uA
    		TSI_SCANC_REFCHRG(15) |		// Set reference osc charge current to 32 uA
    		TSI_SCANC_SMOD(10) |		// Set scan period modulus to 10
    		TSI_SCANC_AMPSC(1) |		// Divide input clock by 2
    		TSI_SCANC_AMCLKS(0));		// Set active mode clock source to LPOSCCLK

    //TSI0_GENCS |= TSI_GENCS_LPCLKS_MASK; // Set low power clock source to VLPOSCCLK

/* Electrode E1 is aligned with the orange LED */
#define Electrode_E1_EN_MASK TSI_PEN_PEN5_MASK
/* Electrode E2 is aligned with the yellow LED */
#define Electrode_E2_EN_MASK TSI_PEN_PEN8_MASK
/* Electrode E3 is aligned with the green LED */
#define Electrode_E3_EN_MASK TSI_PEN_PEN7_MASK
/* Electrode E4 is aligned with the blue LED */
#define Electrode_E4_EN_MASK TSI_PEN_PEN9_MASK

    TSI0_PEN = Electrode_E1_EN_MASK | Electrode_E2_EN_MASK |
    		Electrode_E3_EN_MASK | Electrode_E4_EN_MASK;
    
    /* In low power mode only one pin may be active; this selects electrode E4 */
    TSI0_PEN |= TSI_PEN_LPSP(9);
    TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;  // Enables TSI
}

/* Calibrate the capacitive touch sensors */
void TSI_Calibrate(void) {
	int i;
	uint16_t baseline;
	
	TSI0_GENCS |= TSI_GENCS_SWTS_MASK;	/* Software Trigger Start */
	while(!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    for(i = 0; i < ELECTRODE_COUNT; i++) {
        baseline = *(electrodeHW[i].counterRegister);
        electrodeHW[i].threshold = baseline + THRESHOLD_OFFSET;
    }
}
	
int electrode_in(int electrodeNumber) {
	uint16_t oscCount;
	if(ELECTRODE_COUNT <= electrodeNumber){
		my_printf("ERROR: Incorrect electrode number for touch pad passed");
		return -1;
	}
	TSI0_GENCS |= TSI_GENCS_SWTS_MASK;	/* Software Trigger Start */
	while(!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    oscCount = *(electrodeHW[electrodeNumber].counterRegister);

    /* Returns 1 when pushbutton is depressed and 0 otherwise */
	
	return oscCount > electrodeHW[electrodeNumber].threshold;
}
