#ifndef UTILITY_H
#define UTILITY_H
#include <errno.h>
#include "derivative.h" 
#include <stdio.h>
#include "lcdc.h"
#include "lcdcConsole.h"

_EWL_IMP_EXP_C int _EWL_CDECL fprintf(FILE *_EWL_RESTRICT stream, const char *_EWL_RESTRICT format, ...) _EWL_CANT_THROW; 
/*typedef unsigned int uint32_t;/\*depending on if using 32 bit addressing or 64 bit addressing*/

#define FALSE 0
#define TRUE 1

//struct console console;

int equal_strings(const char* str1, const char* str2);
int string_length(const char* str);
void string_copy(char* str_dst, const char* str_src);
void string_copy_n(char* str_dst, const char* str_src, const int num);

/*Check if the largeStr has the smallStr as prefix*/
int prefix_string(const char* largeStr, const char* smallStr);
void* getPtrFromStr(const char* str);
int getIntFromStr(const char* str);

char text_buffer[1024];
void my_printf(const char* str);
void my_printf_1(char* formatStr, const char* arg);
void my_printf_1int(char* formatStr, int arg);
char uart_fgetc();
void uart_fputc(char ch);
void uart_init();
#endif
