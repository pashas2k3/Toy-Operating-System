/**
 * svc.h
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _SVC_H
#define _SVC_H

#define SVC_MaxPriority 15
#define SVC_PriorityShift 4

// Implemented SVC numbers
void svcInit_SetSVCPriority(unsigned char priority);
void svcHandler(void);

enum SVCId{SVC_MALLOC,SVC_FREE, SVC_FOPEN, 
		SVC_FCLOSE, SVC_FGETC, SVC_FPUTC, SVC_CREATE,
		SVC_DELETE, SVC_LED_SIGNAL, SVC_SWITCH_IN, SVC_LCD_DISPLAY,
		SVC_POTENTIOMETER_IN, SVC_THERMISTOR_IN, SVC_TOUCH_SENSOR_IN,
		SVC_UART_DISPLAY, SVC_UART_IN};

/*Memory Management*/
void* SVCMalloc(unsigned int size);
void SVCFree( void* ptr);

/*File Management*/
void SVCCreate(const char* path, int isDirectory);
void SVCDelete(const char* path);

void SVCFPutC( const char ch, void* stream);
char SVCFGetC( void* stream);

void* SVCFOpen(const char* path);
void SVCFClose(void* stream);

/*HW management*/
void SVCLedSignal(int* ledValues);
void SVCSwitchIn(int* switchValues);
void SVCLCDDisplay(const char ch);
int SVCPotentiometerIn();
int SVCThermistorIn();
void SVCTouchSensorIn(int* touchSensorArray);

/*UART I/O*/
char SVCUartIn();
void SVCUartDisp(char ch);
#endif /* ifndef _SVC_H */
