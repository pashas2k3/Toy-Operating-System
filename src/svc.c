/**
 * svc.c
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */
#include "memory_manager.h"
/*#include <sys/time.h>*/
#include "utility.h"
#include "file_system.h"
#include "svc.h"
/*
 * Size of the supervisor call stack frame (no FP extension):
 *   No alignment => 32 (0x20) bytes
 *   With alignment => 36 (0x24) bytes
 *   
 * Format of the supervisor call stack frame (no FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +32)
 *   +32		 Possible Reserved Word for Alignment on 8 Byte Boundary
 *   
 * Size of the supervisor call stack frame (with FP extension):
 *   No alignment => 104 (0x68) bytes
 *   With alignment => 108 (0x6C) bytes
 *   
 * Format of the supervisor call stack frame (with FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +104)
 *   +32		 S0
 *   +36		 S1
 *   +40		 S2
 *   +44		 S3
 *   +48		 S4
 *   +52		 S5
 *   +56		 S6
 *   +60		 S7
 *   +64		 S8
 *   +68		 S9
 *   +72		 S10
 *   +76		 S11
 *   +80		 S12
 *   +84		 S13
 *   +88		 S14
 *   +92		 S15
 *   +96		 FPSCR
 *   +100		 Reserved Word for 8 Byte Boundary of Extended Frame
 *   +104		 Possible Reserved Word for Alignment on 8 Byte Boundary
 */

#include <derivative.h>
#include <stdio.h>
#include "svc.h"

#define XPSR_FRAME_ALIGNED_BIT 9
#define XPSR_FRAME_ALIGNED_MASK (1<<XPSR_FRAME_ALIGNED_BIT)

struct frame {
	union {
		int r0;
		int arg0;
		int returnVal;
	};
	union {
		int r1;
		int arg1;
	};
	union {
		int r2;
		int arg2;
	};
	union {
		int r3;
		int arg3;
	};
	int r12;
	union {
		int r14;
		int lr;
	};
	int returnAddr;
	int xPSR;
};


/* Issue the SVC (Supervisor Call) instruction (See A7.7.175 on page A7-503 of the
 * ARM師7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3 (ID100710)) */


#define SVC_WRAPPER(id, returnType, fcnName , ...) \
	returnType __attribute__((naked)) __attribute__((noinline)) fcnName (__VA_ARGS__){\
	__asm("svc %0" : : "I" (id));\
	__asm("bx lr");\
	}\
	
/*Memory management*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
SVC_WRAPPER(SVC_MALLOC, void*, SVCMalloc, unsigned int size)
#pragma GCC diagnostic pop
SVC_WRAPPER(SVC_MALLOC, void, SVCFree, void* ptr)

/*File Management*/
SVC_WRAPPER(SVC_CREATE, void, SVCCreate, const char* path, int isDirectory)
SVC_WRAPPER(SVC_DELETE, void, SVCDelete, const char* path)

SVC_WRAPPER(SVC_FPUTC, void, SVCFPutC, const char ch, void* stream)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
SVC_WRAPPER(SVC_FGETC, char, SVCFGetC, void* stream)
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
SVC_WRAPPER(SVC_FOPEN, void*, SVCFOpen, const char* path)
#pragma GCC diagnostic pop

SVC_WRAPPER(SVC_FCLOSE, void, SVCFClose, void* stream)

/*Hardware I/O*/
SVC_WRAPPER(SVC_LED_SIGNAL, void, SVCLedSignal, int* ledValues)
SVC_WRAPPER(SVC_SWITCH_IN, void, SVCSwitchIn, int* switchValues)
SVC_WRAPPER(SVC_LCD_DISPLAY, void, SVCLCDDisplay, const char ch)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
SVC_WRAPPER(SVC_POTENTIOMETER_IN, int, SVCPotentiometerIn, void)
SVC_WRAPPER(SVC_THERMISTOR_IN, int, SVCThermistorIn, void)
#pragma GCC diagnostic pop

SVC_WRAPPER(SVC_TOUCH_SENSOR_IN, void, SVCTouchSensorIn, int* touchSensorAray)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
SVC_WRAPPER(SVC_UART_IN, char, SVCUartIn, void);
#pragma GCC diagnostic pop
SVC_WRAPPER(SVC_UART_DISPLAY,void, SVCUartDisp,char ch);

/*May need pragma's for the function calls*/

/* This function sets the priority at which the SVCall handler runs (See
 * B3.2.11, System Handler Priority Register 2, SHPR2 on page B3-723 of
 * the ARM師7-M Architecture Reference Manual, ARM DDI 0403Derrata
 * 2010_Q3 (ID100710)).
 * 
 * If priority parameter is invalid, this function performs no action.
 * 
 * The ARMv7-M Architecture Reference Manual in section "B1.5.4 Exception
 * priorities and preemption" on page B1-635 states, "The number of
 * supported priority values is an IMPLEMENTATION DEFINED power of two in
 * the range 8 to 256, and the minimum supported priority value is always 0.
 * All priority value fields are 8-bits, and if an implementation supports
 * fewer than 256 priority levels then low-order bits of these fields are RAZ."
 * 
 * In the K70 Sub-Family Reference Manual in section "3.2.2.1 Interrupt
 * priority levels" on page 85, it states, "This device supports 16 priority
 * levels for interrupts.  Therefore, in the NVIC each source in the IPR
 * registers contains 4 bits."  The diagram that follows goes on to confirm
 * that only the high-order 4 bits of each 8 bit field is used.  It doesn't
 * explicitly mention the System Handler (like the SVC handler) priorities,
 * but they should be handled consistently with the interrupt priorities.
 * 
 * It is important to set the SVCall priority appropriately to allow
 * or disallow interrupts while the SVCall handler is running. */

void svcInit_SetSVCPriority(unsigned char priority) {
	if(priority > SVC_MaxPriority)
		return;

	SCB_SHPR2 = (SCB_SHPR2 & ~SCB_SHPR2_PRI_11_MASK) |
			SCB_SHPR2_PRI_11(priority << SVC_PriorityShift);
}

void svcHandlerInC(struct frame *framePtr);

/* Exception return behavior is detailed in B1.5.8 on page B1-652 of the
 * ARM師7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710) */

/* When an SVC instruction is executed, the following steps take place:
 * (1) A stack frame is pushed on the current stack (either the main or
 *     the process stack, depending on how the system is configured)
 *     containing the current values of R0-R3, R12, LR, the return
 *     address for the SVC instruction, xPSR, and, if appropriate, the
 *     floating point registers,
 * (2) An appropriate value is loaded into the LR register indicating
 *     that a stack frame is present on the stack (this will cause a
 *     special return sequence later when this value is loaded into
 *     the PC),
 * (3) Values of R0-R3 and R12 are no longer valid,
 * (4) The PC is set to the address in the interrupt vector table for
 * 	   the interrupt service routine for the SVC instruction,
 * (5) The processor switches to Handler mode (code execution in
 *     Handler mode is always privileged),
 * (6) The xPSR is set to indicate appropriate SVC state,
 * (7) The processor switches to the main stack by now using the main
 * 	   stack pointer.
 *     
 * These steps are discussed in detail in the pseudo-code given for
 * processor action ExceptionEntry() on page B1-643 of the ARM師7-M
 * Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710).  ExceptionEntry() invokes PushStack() and
 * ExceptionTaken() on page B1-643. */

/* The following assembler function makes R0 point to the top-of-stack
 * for the stack that was current (either the main or the process stack)
 * before the SVC interrupt service routine began running.  This is
 * where the stack frame was stored by the SVC instruction.  Then, this
 * function calls the svcHandlerInC function.  Note that when a C
 * function is called, R0 contains the first parameter.  Therefore, the
 * first parameter passed to svcHandlerInC is a pointer to the
 * top-of-stack of the stack containing the stack frame. */

void __attribute__((naked)) svcHandler(void) {
	__asm("\n\
            tst		lr, #4\n\
			ite		eq\n\
			mrseq	r0, msp\n\
			mrsne	r0, psp\n\
			push	{lr}\n\
			bl		svcHandlerInC\n\
			pop		{pc}\n\
			");
}
/*ASSUMPTION - 32 bit addressing looking at the return address being stored on 
 * int */
void svcHandlerInC(struct frame* framePtr)
{
	switch(((unsigned char *)framePtr->returnAddr)[-2]) {
	/*Memory management*/
	case SVC_MALLOC: 
		framePtr->returnVal = (int)myMalloc((unsigned int)framePtr->arg0); break;
	case SVC_FREE:
		myFree((void*)framePtr->arg0); break;
		/*File Management*/
	case SVC_CREATE:
		my_fcreate((const char*)framePtr->arg0, framePtr->arg1); break;
	case SVC_DELETE:
		my_fdelete((const char*)framePtr->arg0);break;
		
	case SVC_FPUTC:
		my_fputc((const char)framePtr->arg0, (void*)framePtr->arg1);break;
	case SVC_FGETC:
		framePtr->returnVal = my_fgetc( (void*)framePtr->arg0);break;
		
	case SVC_FOPEN:
		framePtr->returnVal = (int)my_fopen((const char*) framePtr->arg0);break;
	case SVC_FCLOSE:
		my_fclose((void*)framePtr->arg0); break;
		
	case SVC_LED_SIGNAL:
		setLedValues((int*)framePtr->arg0); break;
	case SVC_SWITCH_IN:
		getSwitchValues((int*)framePtr->arg0);break;
	case SVC_LCD_DISPLAY:
		setLCDDisplay((const char)framePtr->arg0);break;
	case SVC_POTENTIOMETER_IN:
		framePtr->returnVal = getPotentiometerValue();break;
	case SVC_THERMISTOR_IN:
		framePtr->returnVal = getThermistorValue();break;
	case SVC_TOUCH_SENSOR_IN:
		getTouchSensorValue((int*)framePtr->arg0);break;
	case SVC_UART_DISPLAY:
		uart_fputc((char)framePtr->arg0); break;
	case SVC_UART_IN:
		framePtr->returnVal = uart_fgetc();	break;
	default:
		printf("Unknown SVC has been called\n");exit(1);break;
	}
}

