#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memory_manager.h"
/*#include <sys/time.h>*/
#include "utility.h"
#include "file_system.h"
#include "priv.h"
#include "svc.h"
#include "mcg.h"
#include "sdram.h"
#include "uart.h"

const int LINE_MAX = 256;

/*******UTILITIES*******/
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
#define FALSE 0
#define TRUE 1
static void cleanup_memory(int argc, char** argv)
{
  if(argv != NULL){
    int counter = 0;
    for(;counter < argc ; counter++){
      SVCFree(argv[counter]);
      argv[counter] = NULL;
    }/*  end for */
    SVCFree(argv);
  }/*  end -if */
}

/*****ERROR MANAGEMENT*******/
#define FOREACH_ERROR_CONDN(ERROR_CONDN) \
        ERROR_CONDN(SUCCESS)   \
        ERROR_CONDN(BUFFER_EXCEEDED)  \
        ERROR_CONDN(PARSE_ERROR)   \
        ERROR_CONDN(READ_ERROR)  \
	ERROR_CONDN(BUG_ERROR)  \
	ERROR_CONDN(CMD_NOT_FOUND) \
	ERROR_CONDN(INCORRECT_NUM_PARAMS) \
	ERROR_CONDN(GET_TIME_OF_DAY_CALL_ERROR) \
	ERROR_CONDN(INCORRECT_ARGUMENT)\
	ERROR_CONDN(DATE_CALCULATION_BUG)\
	ERROR_CONDN(OUT_OF_MEMORY)\
	ERROR_CONDN(C_API_ERROR)\
	ERROR_CONDN(FILE_NOT_FOUND)\
	ERROR_CONDN(ONE_CHAR_ONLY_ACCEPTED)\
	ERROR_CONDN(UNKNOWN_INPUT)\

enum ERROR_CONDN_ENUM {
    FOREACH_ERROR_CONDN(GENERATE_ENUM)
};

static const char *ERROR_CONDN_STRING[] = {
    FOREACH_ERROR_CONDN(GENERATE_STRING)
};

static int process_error(int error_enum)
{
  if (error_enum>SUCCESS){
	sprintf(text_buffer, "ERROR: %s\n",ERROR_CONDN_STRING[error_enum]);
    my_printf(text_buffer);
  }
  return error_enum;
}
/****processing_word should be unconditional****/
/**************PARSE STRING********************************/
#define MAXIMUM_NUM_CHARS 256

static int parse_input_string( int* argc_ref, char*** argv_ref)
{
  static char text_buffer[MAXIMUM_NUM_CHARS];
  int length_buffer = 0;
  int processing_word = FALSE;
  int result = SUCCESS;

  /*  The number of word in the buffer cannot be more than */
  /*  (maximum_num_chars/2) */
  /* NOTE: Could use char arrays instead of int arrays 
     as max number of chars is 256 */
  static int word_start[MAXIMUM_NUM_CHARS];
  static int word_end[MAXIMUM_NUM_CHARS];

  /*  fprintf "$ "(with a space to count whitespace to non-whitespace transitions) */
  my_printf("$ ");

  /* 1st iteration get the full string into a buffer */
  /*  get the whitespace to not whitespace transitions */
  {
    char curr_ch = 0;
    while(curr_ch != '\n' && curr_ch != '\r'){
      if (length_buffer > (MAXIMUM_NUM_CHARS)) {
	result = BUFFER_EXCEEDED;
	goto EXIT_POINT;
      }
	  curr_ch = SVCUartIn();
      text_buffer[length_buffer++] = curr_ch;

  	// Echo the input character back to the UART
      SVCUartDisp(curr_ch);
  	//uartPutchar(UART2_BASE_PTR, curr_ch);

//  	// Output the character on the TWR_LCD_RGB
//  	lcdcConsolePutc(&console, curr_ch);

  	if(curr_ch == ' ' || curr_ch == '\t') {
  		if(processing_word) {
  			word_end[(*argc_ref)-1] = length_buffer-1;
	}
	processing_word = FALSE;
      } else {
	if(!processing_word)
	  word_start[(*argc_ref)++] = length_buffer-1;
		processing_word = TRUE;
      }

  if(ferror(stdin)) {
	result = READ_ERROR;
	goto EXIT_POINT;
      }
    }

    if(processing_word){
    	word_end[(*argc_ref)-1] = length_buffer-1;
    }
  }

  /*If *argc_ref is 1 but word_start == word_end,
   it is an empty list*/
  if(*argc_ref == 1 && word_start[0] == word_end[0]) {
    *argc_ref = 0;
    goto EXIT_POINT;
  }

  /*  Allocate the (*argv_ref)  */
  (*argv_ref) = SVCMalloc(sizeof(char*)*(*argc_ref));
  if(NULL == *argv_ref){
    result = OUT_OF_MEMORY;
    goto EXIT_POINT;
  }

  /* 2nd iteration- iterate over word start and ends identified */

  {
    int word_counter = 0;
    while(word_counter < (*argc_ref)){
      if(word_end[word_counter] < word_start[word_counter]){
	result = BUG_ERROR;
	goto EXIT_POINT;
      }

      /*  Allocate space for word on (*argv_ref) array */
      /*  should include space for one null character in end */
      (*argv_ref)[word_counter] = SVCMalloc(sizeof(char)*
				  (word_end[word_counter] -
				   word_start[word_counter]+1));
      if(NULL == (*argv_ref)[word_counter]){
	result = OUT_OF_MEMORY;
	goto EXIT_POINT;
      }

      {
	int curr_seg_counter = 0;
	int curr_ch_counter = word_start[word_counter];
	while(curr_ch_counter < (word_end[word_counter])){
	  (*argv_ref)[word_counter][curr_seg_counter] =
	    text_buffer[curr_ch_counter];
	  curr_ch_counter++;
	  curr_seg_counter++;
	}

	(*argv_ref)[word_counter][curr_seg_counter] = '\0';
      }

      /*  Move onto the next word in list */
      word_counter++;
    }
  }

  my_printf("$ ");
  my_printf(text_buffer);
  /*  EXTRA - any valid escape sequences encountered- add an extra backslash before storing/displaying it */

 EXIT_POINT:
  return process_error(result);
}

/**************PROCESS COMMAND********************************/
#define NUM_SECONDS_IN_DAY 86400
#define NUM_DAYS_IN_YEAR_UNADJUSTED 365
#define EPOCH_YEAR 1970
#define NUM_MONTHS_YEAR 12
#define NUM_MINUTES_IN_HOUR 60
#define NUM_HOUR_IN_DAY 24
#define NUM_SECONDS_IN_MINUTE 60

#define DEC_DAY_INC_MONTH_IF_NEEDED(num_days, days, month)	\
  { if (days > num_days)					\
      { day -= num_days;					\
	month++;}}						\

static int cmd_date(int argc, char *argv[])
{
	  int result = SUCCESS;
	  /*static const char* months[]= {"January","February",
				"March","April","May","June",
				"July","August","September",
				"October","November","December"};

  struct timeval tv;
  struct timezone tz;
  if (argc != 1) {
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  if(gettimeofday(&tv, &tz)){
    result = GET_TIME_OF_DAY_CALL_ERROR;
    goto EXIT_POINT;
  }

   /\*DEBUGGING*\/ 
   tv.tv_sec  = 86399; 

   tv.tv_sec = 1441571494; /\*Sep 6th*\/ 

   tv.tv_sec =  94768384;/\*Jan 1 1973*\/ 
   tv.tv_sec = 1441571494;/\*Today's date*\/ 

   tv.tv_sec = 32999585010;/\*Sep 1 3015*\/ 

  {
    Since epoch is Jan 1 0000 hours 1970, the only part we
      really need to be careful about is the month and year.
      Rest are pretty well defined
    unsigned int fractional_secs = tv.tv_usec;
    unsigned int num_minutes_since_epoch = tv.tv_sec/NUM_SECONDS_IN_MINUTE;
    unsigned int time_sec = tv.tv_sec % NUM_SECONDS_IN_MINUTE;
    {Adjust for the timezone in the hours elapsed
      num_minutes_since_epoch -= tz.tz_minuteswest;
    }
    unsigned int num_hours_since_epoch = num_minutes_since_epoch/NUM_MINUTES_IN_HOUR;
    unsigned int time_min = num_minutes_since_epoch % NUM_MINUTES_IN_HOUR;


    unsigned int num_days_since_epoch = num_hours_since_epoch/ NUM_HOUR_IN_DAY;
    unsigned int time_hours = num_hours_since_epoch % NUM_HOUR_IN_DAY;

    int year = 0;
    int day = 1;
    int month = 0;

    Do leap year adjustments
    unsigned int delta_year = num_days_since_epoch/
      NUM_DAYS_IN_YEAR_UNADJUSTED;

    Check for intervening leap years by
      divisiblity by 4/100/400
    int divBy4Years =
      ((EPOCH_YEAR + delta_year)/4)- (EPOCH_YEAR/4);
    int divBy100Years =
      ((EPOCH_YEAR + delta_year)/100)- (EPOCH_YEAR/100);
    int divBy400Years =
      ((EPOCH_YEAR + delta_year)/400)- (EPOCH_YEAR/400);
    int num_intervening_leap_years =
      divBy4Years - divBy100Years + divBy400Years;

    if(divBy4Years< 0 ||
       divBy100Years < 0 ||
       divBy400Years < 0 ||
       num_intervening_leap_years < 0) {
      result = DATE_CALCULATION_BUG;
      goto EXIT_POINT;
    }


    Cut the number of days from pre-adjusted days into
      current year
    int days_into_current_year = num_days_since_epoch %
      NUM_DAYS_IN_YEAR_UNADJUSTED;

    if(days_into_current_year > NUM_DAYS_IN_YEAR_UNADJUSTED+ 1){
      result = DATE_CALCULATION_BUG;
      goto EXIT_POINT;
    }

    If it goes into negative, wrap around the days and
      adjust the year
    days_into_current_year -= num_intervening_leap_years;
    {
      Exclude current year (pre-adjustment)
	if it is a leap year too
      int current_year_is_leap_year =
	(EPOCH_YEAR + delta_year)%4 == 0 &&
	((EPOCH_YEAR + delta_year)%100 != 0 ||
	 (EPOCH_YEAR + delta_year)%400 != 0);

      Adjust the days deficit accounted to past years if
	current leap year was included into it
      days_into_current_year += current_year_is_leap_year;
    }

    if(days_into_current_year < 0){
      days_into_current_year += NUM_DAYS_IN_YEAR_UNADJUSTED;
      delta_year--;
    }
    year = delta_year + EPOCH_YEAR;

    Get the number of months after checking if current
      year is a leap year
    day += days_into_current_year;

    January
    DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    February
    If current year is leap year, be sure to take that
      into account
    if(month == 1){
      if(year%4 == 0 && (year%100 != 0 || year%400 != 0)){
	DEC_DAY_INC_MONTH_IF_NEEDED(29, day, month);
      }else{
	DEC_DAY_INC_MONTH_IF_NEEDED(28, day, month);
      }
    }

    March
    if(month == 2)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    April
    if(month == 3)
      DEC_DAY_INC_MONTH_IF_NEEDED(30, day, month);

    May
    if(month == 4)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    June
    if(month == 5)
      DEC_DAY_INC_MONTH_IF_NEEDED(30, day, month);

    July
    if(month == 6)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    August
    if(month == 7)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    September
    if(month == 8)
      DEC_DAY_INC_MONTH_IF_NEEDED(30, day, month);

    October
    if(month == 9)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    November
    if(month == 10)
      DEC_DAY_INC_MONTH_IF_NEEDED(30, day, month);

    December
    if(month == 11)
      DEC_DAY_INC_MONTH_IF_NEEDED(31, day, month);

    Post December any days left is a bug condition
    if(month == NUM_MONTHS_YEAR){
      result = DATE_CALCULATION_BUG;
      goto EXIT_POINT;
    }
    Print format: January 23, 2014 15:57:07.123456

    printf( "Current date is %s %d, %d %d:%d:%d.%d\n",
	  months[month], year, day,
	  time_hours, time_min, time_sec, fractional_secs);
  }
  EXIT_POINT:
*/
  return result;
}

static int cmd_echo(int argc, char *argv[])
{
  int counter = 1;/*Skipping the first one*/
  int result = SUCCESS;

  for(;counter < argc; counter++){
    sprintf(text_buffer,"%s ", argv[counter]);
    my_printf(text_buffer);
  }
  my_printf("\n");

/* EXIT_POINT:*/
  return result;
}

static int cmd_exit(int argc, char *argv[])
{
  int result = SUCCESS;
  if(argc != 1){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  exit(0);

 EXIT_POINT:
  return result;
}

static int cmd_help(int argc, char *argv[]);
 static int cmd_uname(int argc, char *argv[])
 {
  int result = SUCCESS;
  if(argc != 1){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  my_printf("Unix Variant\n");
 EXIT_POINT:
  return result;
}

static int cmd_malloc(int argc, char *argv[])
{
  int result = SUCCESS;
  unsigned int memSize = 0;
  void* ptr = NULL;

  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  memSize = (unsigned int)getPtrFromStr(argv[1]);
  if(errno) {
    result = C_API_ERROR;
    goto EXIT_POINT;
  }

  /*Allocate memory*/
  ptr = SVCMalloc(memSize);

  if(NULL == ptr){
    result = OUT_OF_MEMORY;
    goto EXIT_POINT;
  } else {
    sprintf( text_buffer, "Allocated memory at %p\n", ptr);
    my_printf(text_buffer);
  }

  /*print out the address */
 EXIT_POINT:
  return result;
}

static int cmd_free(int argc, char *argv[])
{
  int result = SUCCESS;
  void* ptr = NULL;

  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  ptr = getPtrFromStr(argv[1]);
  if(errno) {
    result = C_API_ERROR;
    goto EXIT_POINT;
  }

  /*Allocate memory*/
  SVCFree(ptr);

  /*print out that free operation completed */
  my_printf("Free call completed\n");
 EXIT_POINT:
  return result;
}

static int cmd_memory_map(int argc, char *argv[])
{
  int result = SUCCESS;
  if(argc != 1){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  memoryMap();
 EXIT_POINT:
  return result;
}

static int cmd_fopen(int argc, char *argv[])
{
  int result = SUCCESS;
  void* fileStructStream = NULL;
  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  fileStructStream = SVCFOpen(argv[1]);

  if (NULL == fileStructStream){
    result = FILE_NOT_FOUND;
    goto EXIT_POINT;
  }
  
  sprintf(text_buffer, "Opened file %p\n",fileStructStream);
  my_printf(text_buffer);
 EXIT_POINT:
  return result;
}

static int cmd_fclose(int argc, char *argv[])
{
  int result = SUCCESS;
  void* ptr = NULL;

  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  ptr = getPtrFromStr(argv[1]);
  if(errno) {
    result = C_API_ERROR;
    goto EXIT_POINT;
  }

  SVCFClose(ptr);

 EXIT_POINT:
  return result;
}

static int cmd_fgetc(int argc, char *argv[])
{
  int result = SUCCESS;
  char ch = 0;
  void* ptr = NULL;
  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  ptr = getPtrFromStr(argv[1]);
  if(errno) {
    result = C_API_ERROR;
    goto EXIT_POINT;
  }
  ch = SVCFGetC(ptr);

  sprintf( text_buffer, "Character: %d\n",ch);
  my_printf(text_buffer);
 EXIT_POINT:
  return result;
}

static int cmd_fputc(int argc, char *argv[])
{
  int result = SUCCESS;
  char ch = 0;
  void* ptr = NULL;
  if(argc != 3){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  
  if (string_length(argv[1])>2 || 
      (string_length(argv[1])==2 && argv[1][1] != 0)) {
    result = ONE_CHAR_ONLY_ACCEPTED;
    goto EXIT_POINT;
  }

  ch = *(argv[1]);
  
  ptr = getPtrFromStr(argv[2]);
  if(errno) {
    result = C_API_ERROR;
    goto EXIT_POINT;
  }
  SVCFPutC(ch, ptr);

 EXIT_POINT:
  return result;
}

static int cmd_create(int argc, char *argv[])
{
  int result = SUCCESS;
  int isDirectory = FALSE;
  if(argc != 3){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  
  if(equal_strings(argv[2],"dir")){
    isDirectory = TRUE;
  } else if (!equal_strings(argv[2],"file")){
    result = UNKNOWN_INPUT;
    goto EXIT_POINT;
  }
  
  SVCCreate(argv[1], isDirectory);
 EXIT_POINT:
  return result;
}

static int cmd_delete(int argc, char *argv[])
{
  int result = SUCCESS;
  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  
  SVCDelete(argv[1]);
 EXIT_POINT:
  return result;
}


static int cmd_ser2lcd(int argc, char* argv[])
{
  int result = SUCCESS;
  char ch = 0;
  if(argc != 1){
	result = INCORRECT_NUM_PARAMS;
	goto EXIT_POINT;
  }
  while(ch != 4){/*ctrl D input character*/
	  ch = SVCUartIn();
	  if(ch != '\n' && ch != '\r')
	  {
		  SVCUartDisp(ch);
		  SVCLCDDisplay(ch);		  
	  } else {
		  SVCUartDisp(ch);
		  SVCLCDDisplay(ch);
	  }

  }
 EXIT_POINT:
  return result;	
}

static int cmd_touch2led(int argc, char* argv[])
{
  int result = SUCCESS;
  int sensorValue[4]= {0,0,0,0};
  if(argc != 1){
	result = INCORRECT_NUM_PARAMS;
	goto EXIT_POINT;
  }
  while (!(sensorValue[0]&& sensorValue[1] && 
		  sensorValue[2] && sensorValue[3]))
  {
	  SVCTouchSensorIn(sensorValue);
	  SVCLedSignal(sensorValue);  
  }

 EXIT_POINT:
  return result;	
}

static int cmd_pot2ser(int argc, char* argv[])
{
	int result = SUCCESS;
	int potValue  = 0;
	if(argc != 1){
		result = INCORRECT_NUM_PARAMS;
		goto EXIT_POINT;
	}
	do{
	  potValue = SVCPotentiometerIn();
	  sprintf(text_buffer,"%d\n", potValue);
	  my_printf(text_buffer);
	}while(potValue != 0);

 EXIT_POINT:
  return result;
}

static int cmd_therm2ser(int argc, char* argv[])
{
	int result = SUCCESS; int sw[2] = {0,0};  
	char* curr_text_ptr = NULL;
	if(argc != 1){
		result = INCORRECT_NUM_PARAMS;
		goto EXIT_POINT;
	}

	do{
		// Display the character as a 
		int temp = SVCThermistorIn();
		sprintf(text_buffer, "%d\n\r",temp);
		curr_text_ptr = text_buffer;
		
		do{
			SVCUartDisp(*curr_text_ptr);
		} while(*curr_text_ptr++);
		
		SVCSwitchIn(sw);
	}while(!sw[1]);
	  
 EXIT_POINT:
  return result;
}

static int cmd_pb2led(int argc, char* argv[])
{
	int result = SUCCESS; int sw[2] = {0,0}; 
	int ledValues[] = {0,0,0,0};
	if(argc != 1){
		result = INCORRECT_NUM_PARAMS;
		goto EXIT_POINT;
	}
	do{
		// Display the character as a 
		SVCSwitchIn(sw);
		ledValues[0] = sw[0]; ledValues[1] = sw[1];
		SVCLedSignal(ledValues);
	}while(!(sw[0]&& sw[1]));
	
 EXIT_POINT:
	return result;
}

static int cmd_ls(int argc, char* argv[])
{
  int result = SUCCESS;
  if(argc != 1){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }
  
  my_ls();
 EXIT_POINT:
  return result;
}

/********************************/
/*!!!!!!!!!!
  NOTE: NEED MACRO
  MAGIC FOR CMD NAMES ALSO TO MAKE COMPARISON EASIER*/

#define NUM_COMMANDS 20
struct commandEntry {
  char *name;
  int (*functionp)(int argc, char *argv[]);
} commands[NUM_COMMANDS] = {{"date", cmd_date},
			    {"echo", cmd_echo},
			    {"exit", cmd_exit},
			    {"help", cmd_help},
			    {"uname", cmd_uname},
			    {"malloc", cmd_malloc},
			    {"free", cmd_free},
			    {"memorymap", cmd_memory_map},
			    {"fopen",cmd_fopen},
			    {"fclose",cmd_fclose},
			    {"fgetc",cmd_fgetc},
			    {"fputc",cmd_fputc},
			    {"create",cmd_create},
			    {"delete",cmd_delete},
			    {"ser2lcd",cmd_ser2lcd},
			    {"touch2led", cmd_touch2led},
			    {"pot2ser",cmd_pot2ser},
			    {"therm2ser",cmd_therm2ser},
			    {"pb2led",cmd_pb2led},
			    {"ls", cmd_ls}};


static int cmd_help(int argc, char *argv[])
{
  int result = SUCCESS;
  int cmd_cnt = 0;
  if(argc != 2){
    result = INCORRECT_NUM_PARAMS;
    goto EXIT_POINT;
  }

  /*Print info depending on command identified for second input*/
  {
    int cmd_found = FALSE;

    for(; (cmd_cnt < NUM_COMMANDS) && !cmd_found; cmd_cnt++){
      cmd_found = equal_strings(argv[1], commands[cmd_cnt].name);
    }

    if(!cmd_found){
      result = CMD_NOT_FOUND;
      goto EXIT_POINT;
    }
  }

  switch(cmd_cnt) {
  case 1:
    my_printf("Displays the current date\n");
    break;
  case 2:
    my_printf("Displays entered arguments\n");
    break;
  case 3:
    my_printf("Exits shell\n");
    break;
  case 4:
    my_printf("Provides help\n");
    break;
  case 5:
    my_printf("Provides operating system name\n");
    break;
  case 6:
    my_printf( "Allocates memory off of heap\n");
    break;
  case 7:
    my_printf( "Free memory allocated off of heap\n");
    break;
  case 8:
    my_printf( "Gives a memoryMap\n");
    break;
  case 9:
	my_printf("Opens file/device \n"); 
	break;	
  case 10:
	my_printf("Closes file/device \n"); 
	break;	
  case 11:
	my_printf("Gets char from file \n"); 
	break;	
  case 12:
	my_printf("Sets char on file \n"); 
	break;	
  case 13:
	my_printf("Create a file\n"); 
	break;	
  case 14:
	my_printf("delete a file \n"); 
	break;	
  case 15:
	my_printf("Serial to LCD movement\n");
	break;
  case 16:
	my_printf("Touch sensor to LED \n");
	break;
  case 17:
	my_printf("Potentiometer to serial port\n");
	break;
  case 18:
	my_printf("Thermistor to Serial port\n");
	break;
  case 19:
	my_printf("Push button to LED \n");
	break;
  case 20: 
	my_printf("List all files in hierarchy \n"); 
	break;	
  default:
    result = INCORRECT_ARGUMENT;
    break;
  }

 EXIT_POINT:
  return result;
}

static int process_cmd(int argc, char *argv[])
{
  int result = SUCCESS;
  int cmd_cnt = 0;
  int cmd_found = FALSE;

  if(argc < 1)
    goto EXIT_POINT;

  for(; (cmd_cnt < NUM_COMMANDS) && !cmd_found; cmd_cnt++){
    cmd_found = equal_strings(argv[0], commands[cmd_cnt].name);
  }

  if(!cmd_found){
    result = CMD_NOT_FOUND;
    goto EXIT_POINT;
  }

  /*  Call the relevant command */
  /*  Subtraction done to offset the post-increment */
  result = commands[cmd_cnt-1].functionp(argc, argv);

 EXIT_POINT:
  return process_error(result);
}
/*******************UNIT TESTING******************************/

/*TODO - Automate the string entry part - pass in a string
  and see if that works instead of fgetc*/
/* static void unit_testing() */
/* { */
/*   int argc;  */
/*   char** argv = NULL; */
/* /\*Case 1 - test simple case - space and tabs *\/ */
/*   parse_input_string(&argc, &argv); */
/*   process_cmd(argc, argv); */
/* /\*Case 2 - space and tabs in beggining and ending*\/ */
/* } */

/*
static int utility_testing()
{
  int result = SUCCESS;

  Check string comparison utility works
  if (equal_strings("abc","def"))
    {result = BUG_ERROR; goto EXIT_POINT;}

  if(!equal_strings("abc","abc"))
    {result = BUG_ERROR; goto EXIT_POINT;}

 EXIT_POINT:
  return process_error(result);
}
*/


int main() {	
	/*Hardware setup */
	mcgInit();
	sdramInit();
	uart_init();/*UART kept independent of SVC for now*/

	//svcInit_SetSVCPriority(7);
	
	// make the shell run in unpriviliged mode
	privUnprivileged();

  /* Make stdin be unbuffered*/
  if(setvbuf(stdin, NULL, _IONBF, 0)) {
	sprintf(text_buffer,"setvbuf failed on stdin: %s", strerror(errno));
	my_printf(text_buffer);
  }
  /* Make stdout be unbuffered*/
  if(setvbuf(stdout, NULL, _IONBF, 0)) {
	sprintf(text_buffer, "setvbuf failed on stdout: %s", strerror(errno));
	my_printf(text_buffer);
  }

  while(1){
    int argc_user = 0;
    char** argv_user = NULL;

    if (parse_input_string( &argc_user, & argv_user) != SUCCESS)
      goto CLEANUP;
    if (process_cmd(argc_user, argv_user) != SUCCESS)
      goto CLEANUP;

  CLEANUP:
    cleanup_memory(argc_user, argv_user);
  }  /*end - for*/ 

return 0;
}
