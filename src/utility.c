#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utility.h"
#include "uart.h"
#include "svc.h"

int getIntFromStr(const char* str)
{
  /*translate the input argument integer or hexadecimal to
    size required*/
  static const char HEX_PREFIX[] = "0x";
  int num = -1;
  char* pEnd = NULL;

  errno = 0; /*reinitialize erro no*/
  if(!prefix_string(str, HEX_PREFIX))
    num = strtoul(str, &pEnd,10);
  else
    num = strtoul(str, &pEnd,16);

  if(errno){
    my_printf_1("API Error:%s", strerror(errno));
    num = -1;
  }

  return num;
}
void* getPtrFromStr(const char* str)
{
  /*translate the input argument integer or hexadecimal to
    size required*/
  static const char HEX_PREFIX[] = "0x";
  void* ptr = NULL;
  char* pEnd = NULL;

  errno = 0; /*reinitialize erro no*/
  if(!prefix_string(str, HEX_PREFIX))
    ptr = (void*)strtoul(str, &pEnd,10);
  else
    ptr = (void*)strtoul(str, &pEnd,16);

  if(errno){
    my_printf_1("API Error:%s", strerror(errno));
    ptr = NULL;
  }
  
  return ptr;
}

int prefix_string(const char* largeStr, const char* smallStr)
{
   int counter = 0;

   while(smallStr[counter] != '\0' &&
	 largeStr[counter] != '\0'){
     if (smallStr[counter] != largeStr[counter])
       return FALSE;
     counter++;
   }

   if(smallStr[counter] != '\0')
     return FALSE;

   return TRUE;
}

void string_copy(char* str_dst, const char* str_src)
{
  int counter = 0;
  
  while(str_src[counter]){
    str_dst[counter] = str_src[counter];
    counter++;
  }
  str_dst[counter]= '\0';
}

void string_copy_n(char* str_dst, const char* str_src, const int num)
{
  int counter = 0;
  if(string_length(str_src) < num){
    my_printf_1int("ERROR:Cannot Copy String, less than %d char\n", 
	   string_length(str_src));
    return;
  }

  while(counter < num ){
    str_dst[counter] = str_src[counter];
    counter++;
  }
  str_dst[counter]= '\0';

}

int string_length(const char* str)
{
  int counter = 0;
  while(str[counter++] != 0);
  return counter;
}

int equal_strings(const char* str1, const char* str2)
 {
   int counter = 0;

   while(str1[counter] != '\0' &&
	 str2[counter] != '\0'){
     if (str1[counter] != str2[counter])
       return FALSE;
     counter++;
   }

   if(str1[counter] != '\0'
      || str2[counter] != '\0')
     return FALSE;

   return TRUE;
 }

char uart_fgetc()
{
	return uartGetchar(UART2_BASE_PTR);
}

void uart_fputc(char ch)
{
	uartPutchar(UART2_BASE_PTR, ch);
}

void my_printf(const char* text)
{
	const char* ptr = text;
	do{
		SVCUartDisp(*ptr);
		if(*ptr == '\r'|| *ptr == '\n')
		{
			SVCUartDisp( '\n');
			SVCUartDisp( '\r');
		} 
	}while(*ptr++);
}
void my_printf_1(char* formatString, const char* arg)
{
	sprintf(text_buffer, formatString, arg);
	my_printf(text_buffer);
}
void my_printf_1int(char* formatString, int arg)
{
	sprintf(text_buffer, formatString, arg);
	my_printf(text_buffer);
}

void uart_init()
{
	const int peripheralClock = 60000000;
	const int KHzInHz = 1000;

//	const int IRC = 32000;					/* Internal Reference Clock */
//	const int FLL_Factor = 640;
//	const int moduleClock = FLL_Factor*IRC;
	const int baud = 115200;
	
	uartInit(UART2_BASE_PTR, peripheralClock/KHzInHz, baud);
}

