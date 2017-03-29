#include "memory_manager.h"
#include  "utility.h"
#include "ledManipulation.h"
#include "pot.h"

/************UTILITY FUNCTION****************/
#define FOREACH_ERROR_CONDN(ERROR_CONDN)	\
  ERROR_CONDN(SUCCESS)				\
  ERROR_CONDN(NULL_POINTER)			\
  ERROR_CONDN(FILE_NOT_FOUND)			\
  ERROR_CONDN(UNIMPLEMENTED)			\
  ERROR_CONDN(BUG_FOUND)			\
  ERROR_CONDN(DEVICE_NOT_FOUND)			\
  ERROR_CONDN(ADD_FILE_TO_NON_DIR)		\

#define GENERATE_ENUM(ENUM) ENUM,

#define GENERATE_STRING(STRING) #STRING,
enum ERROR_CONDN_ENUM {
  FOREACH_ERROR_CONDN(GENERATE_ENUM)
};

static const char *ERROR_CONDN_STRING[] = {
  FOREACH_ERROR_CONDN(GENERATE_STRING)
};


static void myAssert(int condn, int error_enum)
{
  if (!condn){
    my_printf_1("FATAL ERROR: %s\n",ERROR_CONDN_STRING[error_enum]);
    exit(1);
  }
}


/**********IDENTIFY WHICH DEVICE IT IS***************/

const char filePath[255];
const char ledPathPrefix[] = "/dev/hw/led";
const char switchPathPrefix[] = "/dev/hw/sw";

const char lcdRGBPrefix[] = "/dev/hw/lcdRGB";
const char potentiometerPrefix[] = "/dev/hw/pot";
const char thermistorPrefix[] = "/dev/hw/thermistor";
const char touchSensorPrefix[] = "/dev/hw/touchSensor";

/*Device classification - major number based */
enum Devices{ FILES, LED, SWITCHES, LCD_RGB, POTENTIOMETER, 
	      THERMISTOR, TOUCH_SENSOR, NUM_DEVICES };
int majorId_from_path(const char* text_buffer)
{
  if(prefix_string(text_buffer, ledPathPrefix)) {
    return LED;
  } else if (prefix_string(text_buffer, switchPathPrefix)){
    return SWITCHES;
  } else if (prefix_string(text_buffer, lcdRGBPrefix)){
    return LCD_RGB;
  } else if(prefix_string(text_buffer,potentiometerPrefix)){
    return  POTENTIOMETER;
  } else if (prefix_string(text_buffer,thermistorPrefix)){
    return THERMISTOR;
  } else if (prefix_string(text_buffer, touchSensorPrefix)){
    return TOUCH_SENSOR;
  }
  return FILES;
}

/********************************************/
static void file_create(const char* path, const int isDirectory);
static void file_delete(const char* path);
static void* file_open(const char* path);
static int file_close(void* structStream);
static uint8_t file_read(void* structStream);
static int file_write(uint8_t val, void* structStream);
static void file_ls();

/* static void* directory_open(const char* path); */
/* static int directory_close(void* structStream); */
/* static uint8_t directory_read(void* structStream); */
/* static int directory_write(uint8_t val, void* structStream); */

static void* led_open(const char* path);
static int led_close(void* structStream);
static uint8_t led_read(void* structStream);
static int led_write(uint8_t val, void* structStream);
static void led_ls();

static void* switch_open(const char* path);
static int switch_close(void* structStream);
static uint8_t switch_read(void* structStream);
static int switch_write(uint8_t val, void* structStream);
static void switch_ls();

static void* lcdrgb_open(const char* path);
static int lcdrgb_close(void* structStream);
static uint8_t lcdrgb_read(void* structStream);
static int lcdrgb_write(uint8_t val, void* structStream);
static void lcdrgb_ls();

static void* pot_open(const char* path);
static int pot_close(void* structStream);
static uint8_t pot_read(void* structStream);
static int pot_write(uint8_t val, void* structStream);
static void pot_ls();

static void* thermistor_open(const char* path);
static int thermistor_close(void* structStream);
static uint8_t thermistor_read(void* structStream);
static int thermistor_write(uint8_t val, void* structStream);
static void thermistor_ls();

static void* touch_open(const char* path);
static int touch_close(void* structStream);
static uint8_t touch_read(void* structStream);
static int touch_write(uint8_t val, void* structStream);
static void touch_ls();

/********************************************/

struct {
  /* void (*create)(const char* path); */
  /* void (*delete)(const char* path); */
  void* (*open)(const char* path);
  int (*close)(void* structStream);
  uint8_t(*read)(void* structStream);
  int(*write)(uint8_t val, void* structStream);
  void (*ls)();
} deviceTypes[NUM_DEVICES] = {
  {file_open, file_close, file_read, 
   file_write, file_ls},
  /*{directory_create, directory_delete, directory_open,directory_close,directory_read,directory_write},*/
  { led_open, led_close, led_read, 
    led_write, led_ls},
  { switch_open, switch_close, 
    switch_read, switch_write, switch_ls},
  {lcdrgb_open, lcdrgb_close, 
   lcdrgb_read, lcdrgb_write, lcdrgb_ls},
  {pot_open, pot_close, 
   pot_read, pot_write, pot_ls},
  {thermistor_open, thermistor_close, 
   thermistor_read, thermistor_write, thermistor_ls},
  {touch_open, touch_close, 
   touch_read, touch_write, touch_ls}};


/***************************************/
/*Minor number based classifications*/

/* /\********DIRECTORY HANDLING***********\/ */
/* struct directory{ */
/*   const char* name; */
/*   int major_num; */
/*   int minor_num; */
/*   struct file** children; */
/*   struct directory* parent; */
/*   struct directory** children; */
/* }; */

/*
  The Arrangement of file structure is shown by following example


  FileStruct (File) -->  FileStruct (Dir) --> FileStruct (File)
  ^                     ^            ^                   ^
  |                     |  +children |                   |
  |      +parent--------+    | |     +--------- +parent  |
  ---------------------------+ +--------------------------

  The memory is arranged sequentially. But the 
  mapping is maintained separately


  This gives a Tree sort of navigability (the root file path "/" will always exist)

*/

/********FILE HANDLING***********/
#define STATIC_FILE_SIZE 32
#define MAX_DIR_CHILDREN 5
const char ROOT_DIR[] = "";
const char ROOT_DIR_PATH[] = "/";
const char FILE_SEPARATOR = '/';

/*Data for a writable file*/
struct FileOnlyInfo{
  unsigned int opened:1;
  unsigned int curr_byte;
  unsigned int trailing_byte;
  char memory[STATIC_FILE_SIZE]; 
  /*Can make this heap allocated too*/
};
typedef struct FileOnlyInfo FileOnlyStruct;

struct File; /*Forward Declaration*/
struct ChildFile{
  struct File* fileRef;
  struct ChildFile* next;
};
typedef struct ChildFile ChildrenFiles;
struct ChildFileList{
  struct ChildFile* top;
}tmp;
typedef struct ChildFileList ChildrenFilesList;

struct File{
  const char* name;
  unsigned int isDirectory:1;
  struct File* parentRef;/*Reference to parent only*/
  union{
    ChildrenFilesList children;
    FileOnlyStruct fileInfo;
  }minor;
  struct File* next;
}; 
typedef struct File FileStruct;

struct {
  FileStruct rootFile ;
}fileList= {ROOT_DIR, TRUE, NULL, {NULL}, NULL};

FileStruct* getFileHead()
{
  return &(fileList.rootFile);
}

FileStruct* createFile(const char* fileName, int isDirectory)
{
  FileStruct* newFile = NULL;
  int strLen = string_length(fileName);
  char* str = (char*)myMalloc(strLen);
  string_copy(str, fileName);
  
  newFile = (FileStruct*)myMalloc(sizeof(FileStruct));
  newFile->name = str;
  if(!isDirectory){
    newFile->minor.fileInfo.curr_byte = 0;
    newFile->minor.fileInfo.trailing_byte = 0;
    /* newFile->minor.fileInfo.memory[0] = 0; */
    newFile->minor.fileInfo.opened = FALSE;
    newFile->isDirectory = FALSE;
  } else {
    newFile->minor.children.top = NULL;
    newFile->isDirectory = TRUE;
  }
  newFile->next = NULL;
  newFile->parentRef = NULL;/*Assign to root directory by default*/

  return newFile;
}

static void free_file(FileStruct* fileStruct)
{
  /* myAssert(fileStruct->next == NULL); */
  myFree((void*)fileStruct->name);
  /* myFree(fileStruct->ptr); */
  myFree(fileStruct);
}

static void addFile(FileStruct* file)
{
  FileStruct* currFile = NULL;
  myAssert(file != NULL, NULL_POINTER);
  currFile = getFileHead();
  while(currFile->next != NULL) {
    currFile = currFile->next;
  }
  currFile->next = file;
}

static void removeChildParentLinkage(FileStruct* currFile)
{
  /* 
     Remove parent references to child 
     Remove child reference to parent
  */
  FileStruct* parentRef = currFile->parentRef;
  ChildrenFiles* childRef = parentRef->minor.children.top;
  ChildrenFiles* prevFile = NULL;

  /*Find child*/
  while(childRef != NULL && childRef->fileRef != currFile) {
    prevFile = childRef;
    childRef= childRef->next;
  }
  
  /*
    Parent has to have reference to this child.
    Otherwise dangling reference or some other bug
  */
  myAssert(childRef != NULL, BUG_FOUND);
  if(prevFile != NULL){
    prevFile->next = childRef->next;
  }else{
    parentRef->minor.children.top = childRef->next;
  }
  childRef->next = NULL;
  myFree(childRef); 
  /*NOTE: If any error condn comes between this state and the 
    part where we free the file - it will lead to file
    system inconsistency*/

  currFile->parentRef = NULL;
}

static FileStruct* removeFile(const char* filename)
{
  FileStruct* currFile = NULL;
  FileStruct* prevFile = NULL;

  currFile = getFileHead();
  while(NULL != currFile &&
	!equal_strings(currFile->name, filename)){
    prevFile = currFile;
    currFile = currFile->next;
  }
  myAssert(currFile != getFileHead(), BUG_FOUND);
  myAssert(NULL != currFile, FILE_NOT_FOUND);
  myAssert(NULL != prevFile, BUG_FOUND);

  /*Root object is always initialized and cannot be removed*/
  removeChildParentLinkage(currFile);  
  prevFile->next = currFile->next;
  currFile->next = NULL;  

  return currFile;
}

static FileStruct* findFile(const char* filename)
{
  FileStruct* currFile = getFileHead();
  while(NULL != currFile &&
	!equal_strings(currFile->name, filename)){
    currFile = currFile->next;
  }
  return currFile;/*will be NULL if not found*/
}

static void addChildParentLink(FileStruct* childFileOrDir,
			       FileStruct* parentDir )
{
  ChildrenFiles* childFiles = NULL, *prevFiles = NULL;
  myAssert(parentDir != NULL &&
	   parentDir->isDirectory, ADD_FILE_TO_NON_DIR);
  childFileOrDir->parentRef = parentDir;
  childFiles = parentDir->minor.children.top;

  while(childFiles != NULL){
    prevFiles = childFiles;
    childFiles = childFiles->next;
  }
  /*Allocate a Node for the reference in the 
    directory children list*/
  childFiles = myMalloc(sizeof(ChildrenFiles));
  childFiles->fileRef = childFileOrDir;
  childFiles->next = NULL;
  
  if (parentDir->minor.children.top == NULL) {
    parentDir->minor.children.top = childFiles;
  }else {
    prevFiles->next = childFiles;
  }
  
  childFileOrDir->parentRef = parentDir;
}

static void file_create(const char* path, int isDirectory)
{
  char* parentDirectoryName = NULL;

  if (findFile(path)== NULL) {
    /* Initialize parent Reference */
    FileStruct* parentRef = NULL;

    /* 3. Find reference to parent directory*/
    if (!prefix_string( path, ROOT_DIR_PATH)){
      my_printf_1( "ERROR: File %s not within root directory \n", path);
    } else if(!equal_strings(ROOT_DIR_PATH, path)){
      /*reverse iterate to find the parent directory name*/
      int counter = string_length(path)-1;/*To account for \0 in end*/
      for(;counter>=0 && (path[counter] != FILE_SEPARATOR) ;
	  counter--);
      /*
	Ignoring the / in the end for naming the file
	and replacing with null character
	1. "/", counter = 0 and string_length = 1 and copy 0 bytes
	2. "/dir/", counter = 4 and string_length = 5 and 
	copy first 4 bytes
      */
      parentDirectoryName = myMalloc(sizeof(char)*(counter+1));
      string_copy_n(parentDirectoryName, path, counter);
      
      if((parentRef = findFile(parentDirectoryName))== NULL) {
	my_printf_1("ERROR: File parent %s does not exist  \n", 
		    parentDirectoryName);
	goto EXIT_POINT;
      }
      {
	/*Add parent and child references to the file/directory*/
	FileStruct* newFile = createFile(path, isDirectory);
	addChildParentLink(newFile, parentRef);
	addFile(newFile);
      }
    } else {
      my_printf_1("ERROR:File %s couldn't be created because of naming \n",
		  path);
      goto EXIT_POINT;
    }
  } else {
    my_printf_1("ERROR:File %s already exists \n", path);
    goto EXIT_POINT;
  }  
 EXIT_POINT:
  myFree(parentDirectoryName);
}

static void file_delete(const char* path)
{
  FileStruct* ptr = findFile(path);
  if ( ptr == NULL) {
    my_printf_1("ERROR:File %s does not exist\n", path);
    return;
  }

  if(ptr == getFileHead()) {
    my_printf("ERROR: Cannot Remove the root directory \n");
    return;
  }
  if (ptr->isDirectory && ptr->minor.children.top != NULL) {
    my_printf_1("ERROR: Directory %s is not empty \n", path);
    return;
  }
  /*TODO - Can we do any other checks to make sure it is a valid 
    file delete*/
  myAssert(ptr == removeFile(path), BUG_FOUND);

  free_file(ptr);
}

static void* file_open(const char* path)
{
  FileStruct* ptr = NULL;
  if(path == NULL){
    my_printf_1("ERROR: File %s does not exist\n", path);    
    goto EXIT_POINT;
  }

  ptr = findFile(path);
  if (ptr == NULL) {
    my_printf_1("ERROR: File %s does not exist\n", path);    
    goto EXIT_POINT;
  }
  if (ptr->isDirectory){
    my_printf_1("ERROR: Directory %s cannot be opened\n", path);  
    ptr = NULL;
    goto EXIT_POINT;
  }
  
  if (ptr->minor.fileInfo.opened ) {
    my_printf_1("ERROR: File %s already opened for reading\n", path);
    ptr = NULL;
  }else{ 
    ptr->minor.fileInfo.opened = TRUE;
  }
 EXIT_POINT:
  return ptr;
}

static int file_close(void* structStream)
{
  FileStruct* tmp = NULL;
  int success = FALSE;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    goto EXIT_POINT;
  }

  tmp = (FileStruct*)structStream;
  if (tmp->isDirectory){
    my_printf_1("ERROR: Directory %s cannot be closed\n", tmp->name);    
    goto EXIT_POINT;
  }
  if(!tmp->minor.fileInfo.opened){
    my_printf_1("ERROR: , File %s hasn't been opened\n", tmp->name);
    goto EXIT_POINT;
  } 

  tmp->minor.fileInfo.opened = FALSE;
  success = TRUE;
 EXIT_POINT:
  return success;
}

static uint8_t file_read(void* structStream)
{
  FileStruct* tmp = NULL;
  char returnChar = 0;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    goto EXIT_POINT;
  }
  
  tmp = (FileStruct*)structStream;
  if (tmp->isDirectory){
    my_printf_1("ERROR: Directory %s cannot be read\n", tmp->name);    
    goto EXIT_POINT;
  }

  if(!tmp->minor.fileInfo.opened){
    my_printf_1("ERROR: File %s hasn't been opened\n", tmp->name);
    goto EXIT_POINT;
  } 

  if (tmp->minor.fileInfo.curr_byte >= STATIC_FILE_SIZE){
    my_printf_1("ERROR:File %s has been read beyond file limit\n", 
		tmp->name);
    goto EXIT_POINT;
  }
  
  if(tmp->minor.fileInfo.curr_byte >= tmp->minor.fileInfo.trailing_byte){
    my_printf_1("ERROR:crossed trailing byte of %s \n", tmp->name);
    goto EXIT_POINT;
  }

  returnChar = tmp->minor.fileInfo.memory[tmp->minor.fileInfo.curr_byte++];
 EXIT_POINT:
  return returnChar;
}

static int file_write(uint8_t val, void* structStream)
{  
  int success = FALSE;
  FileStruct* tmp = NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    goto EXIT_POINT;
  }
  
  tmp = (FileStruct*)structStream;
  if (tmp->isDirectory){
    my_printf_1("ERROR: Directory %s cannot be written to\n", tmp->name);    
    goto EXIT_POINT;
  }

  if(!tmp->minor.fileInfo.opened){
    my_printf_1("ERROR: File %s hasn't been opened\n", tmp->name);
    goto EXIT_POINT;
  } 

  if (tmp->minor.fileInfo.trailing_byte >= STATIC_FILE_SIZE){
    my_printf_1("ERROR:File %s has been written to beyond file limit\n", 
		tmp->name);
    goto EXIT_POINT;
  }
  
  tmp->minor.fileInfo.memory[tmp->minor.fileInfo.trailing_byte++] = val;
  success = TRUE;
 EXIT_POINT:
  return success;
}

static void recursive_ls_dir(FileStruct* ptr)
{
  ChildrenFiles* currChild = NULL;
  myAssert(ptr->isDirectory, BUG_FOUND);
  my_printf_1("Under Directory %s :\n", ptr->name);

  /*1. List all files/directories under it*/
  currChild = ptr->minor.children.top;
  while(currChild != NULL){
    my_printf_1("%s\n", currChild->fileRef->name);
    currChild = currChild->next;
  }

  /*2. Call recursive_ls_dir recursively 
    for all files under it*/
  currChild = ptr->minor.children.top;
  while(currChild != NULL){
    if(currChild->fileRef->isDirectory)
      recursive_ls_dir(currChild->fileRef);
    currChild = currChild->next;
  }
}

static void file_ls()
{
  FileStruct* filePtr = getFileHead();

  my_printf ("\nFILES\n");

  while(NULL != filePtr){
    my_printf_1( "%s \n", filePtr->name);
    filePtr = filePtr->next;
  }
  /*
    For debugging only
    TODO- keep the following part only 
    and remove the prior one
  */
  my_printf("\n FILES IN HIERARCHY \n");
  recursive_ls_dir(getFileHead());
}

/********LED HANDLING***********/
#define NUM_LED 4

#define LED_INIT(color, num) {FALSE, num, led##color##Config,	\
			      led##color##Off, led##color##On}	\

/* Could keep function pointers for each LED */
char LED_GLOBAL_INIT_FLAG = FALSE;
struct LedInfo{
  unsigned int opened:1;
  unsigned int idx:3;
  void (*config)();
  void (*off)();
  void (*on)();
  /* uint8_t state; */
}ledStruct[NUM_LED] = { LED_INIT(Orange, 0),
			LED_INIT(Yellow, 1),
			LED_INIT(Green, 2),
			LED_INIT(Blue, 3)};
typedef struct LedInfo LedStruct;


static void* led_open(const char* path)
{
  int ledIdx = -1;
  void* ledPtr = NULL;
  myAssert(prefix_string(path, ledPathPrefix), BUG_FOUND);
  ledIdx = getIntFromStr(&path[string_length(ledPathPrefix)-1]);

  if (ledIdx < 0 && ledIdx >= NUM_LED) {
    my_printf_1("ERROR:LED Index exceeded with path %s \n", path);
    goto EXIT_POINT;
  }
  if(ledStruct[ledIdx].opened){
    my_printf_1int("ERROR:LED Index already opened:  %d \n", 
		   ledIdx);
    goto EXIT_POINT;
  }  
  
  if(!LED_GLOBAL_INIT_FLAG ){
    ledInitClock();
    LED_GLOBAL_INIT_FLAG  = TRUE;
  }
  ledStruct[ledIdx].opened = TRUE;
  ledStruct[ledIdx].off();
  ledStruct[ledIdx].config();

  ledPtr = &(ledStruct[ledIdx]);

 EXIT_POINT:
  return ledPtr;
}

static int led_close(void* structStream)
{
  LedStruct* ledStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  ledStruct = (LedStruct*)structStream;

  if (ledStruct->idx < 0 && ledStruct->idx >= NUM_LED) {
    my_printf_1int("ERROR:LED Index exceeded with path %d \n", 
		   ledStruct->idx);
    return FALSE;
  }

  if(!ledStruct->opened){
    my_printf_1int("ERROR:LED Index has not been opened:  %d \n", 
		   ledStruct->idx);
    return FALSE;
  }  
  ledStruct->opened = FALSE;
//  ledStruct->off();
  
  return TRUE;
}
static uint8_t led_read(void* structStream)
{
  LedStruct* ledStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  ledStruct = (LedStruct*)structStream;

  if (ledStruct->idx < 0 && ledStruct->idx >= NUM_LED) {
    my_printf_1int("ERROR:LED Index exceeded with path %d \n", 
		   ledStruct->idx);
    return FALSE;
  }

  if(!ledStruct->opened){
    my_printf_1int("ERROR:LED Index has not been opened:  %d \n", 
		   ledStruct->idx);
    return FALSE;
  }  

  my_printf_1int("ERROR:LED Index cannot be read from :  %d \n", 
		 ledStruct->idx);
  /* return ledStruct[ledIdx].state; */
  return FALSE;
}
static int led_write(uint8_t val, void* structStream)
{
  LedStruct* ledStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  ledStruct = (LedStruct*)structStream;

  if (ledStruct->idx < 0 && ledStruct->idx >= NUM_LED) {
    my_printf_1int("ERROR:LED Index exceeded with path %d \n", 
		   ledStruct->idx);
    return FALSE;
  }

  if(!ledStruct->opened){
    my_printf_1int("ERROR:LED Index has not been opened:  %d \n", 
		   ledStruct->idx);
    return FALSE;
  }  

  if(val!=0)//'0')
    ledStruct->on();
  else
    ledStruct->off();
  
  return TRUE;
}

static void led_ls()
{
  int cnt = 0;
  my_printf ( "\nLED\n");

  for(; cnt< NUM_LED; cnt++){
    sprintf(text_buffer,"%s%d\n", ledPathPrefix, ledStruct[cnt].idx);
    my_printf(text_buffer );
  }
}
/********SWITCH HANDLING***********/
#define NUM_SWITCH 2
#define SWITCH_INIT(sw, num) {FALSE,num, sw##Config, sw##In}	\

char SWITCH_GLOBAL_INIT_FLAG = FALSE;
struct SwitchInfo{
  /* unsigned int created:1; */
  unsigned int opened:1;
  unsigned int idx:3;
  void (*config)();
  int (*in)();
} switchStruct[NUM_SWITCH] = {
  SWITCH_INIT(SW1,0),
  SWITCH_INIT(SW2,1)};

typedef struct SwitchInfo SwitchStruct;

static void* switch_open(const char* path)
{
  int switchIdx = -1;
  void* returnPtr = NULL;
  myAssert(prefix_string(path, switchPathPrefix),BUG_FOUND);
  switchIdx = getIntFromStr(&path[string_length(switchPathPrefix)-1]);

  if (switchIdx < 0 && switchIdx >= NUM_SWITCH) {
    my_printf_1("ERROR:SWITCH Index exceeded with path %s \n", path);
    goto EXIT_POINT;
  }
  if(switchStruct[switchIdx].opened){
    my_printf_1("ERROR:SWITCH Index has already been opened:  %s \n", path);
    goto EXIT_POINT;
  }  
  switchStruct[switchIdx].opened = TRUE;
  if(!SWITCH_GLOBAL_INIT_FLAG){
    pushButtonInitClock();
    SWITCH_GLOBAL_INIT_FLAG = TRUE;
  } 
  switchStruct[switchIdx].config();
  returnPtr = &(switchStruct[switchIdx]);

 EXIT_POINT:
  return returnPtr;
}

static int switch_close(void* structStream)
{
  SwitchStruct* switchStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  switchStruct = (SwitchStruct*)structStream;

  if (switchStruct->idx < 0 && switchStruct->idx >= NUM_SWITCH) {
    my_printf_1int("ERROR:SWITCH Index exceeded with path %d \n", 
		   switchStruct->idx);
    return FALSE;
  }

  if(!switchStruct->opened){
    my_printf_1int("ERROR:SWITCH Index has not been opened:  %d \n", 
		   switchStruct->idx);
    return FALSE;
  }  
  switchStruct->opened = FALSE;

  /*switching off SWITCH left to delete call*/
  return TRUE;
}
static uint8_t switch_read(void* structStream)
{
  SwitchStruct* switchStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  switchStruct = (SwitchStruct*)structStream;

  if (switchStruct->idx < 0 && switchStruct->idx >= NUM_SWITCH) {
    my_printf_1int("ERROR:SWITCH Index exceeded with path %d \n", 
		   switchStruct->idx);
    return FALSE;
  }

  if(!switchStruct->opened){
    my_printf_1int("ERROR:SWITCH Index has not been opened:  %d \n", 
		   switchStruct->idx);
    return FALSE;
  }  

  return switchStruct->in();
}

static int switch_write(uint8_t val, void* structStream)
{
  SwitchStruct* switchStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  switchStruct = (SwitchStruct*)structStream;

  if (switchStruct->idx < 0 && switchStruct->idx >= NUM_SWITCH) {
    my_printf_1int("ERROR:SWITCH Index exceeded with path %d\n", 
		   switchStruct->idx);
    (void)val;

    return FALSE;
  }

  if(!switchStruct->opened){
    my_printf_1int("ERROR:SWITCH Index has not been opened:  %d\n" ,switchStruct->idx);
    return FALSE;
  }  

  my_printf("ERROR:SWITCH is read only\n");
  
  return FALSE;
}
static void switch_ls()
{
  int cnt = 0;
  my_printf ( "\nSWITCHES\n");

  for(; cnt< NUM_SWITCH; cnt++){
    sprintf(text_buffer,"%s%d\n", switchPathPrefix, switchStruct[cnt].idx);
    my_printf(text_buffer);
  }
}

/**********************LCD RGB**********************************/
char LCD_GLOBAL_INIT_FLAG = FALSE;
/*If no future item to be added to LCDInfo struct, add more fields to it*/
struct LCDInfo{
  /* unsigned int created:1; */
  unsigned int opened:1;
  struct console console;
} lcdStruct = {FALSE };


static void* lcdrgb_open(const char* path)
{
  if(!equal_strings(path, lcdRGBPrefix))
    {
      /*LCD RGB has only one struct element*/
      my_printf("ERROR: LCD RGB name mismatch\n");
      return NULL;
    }
  if(lcdStruct.opened){
    /*Error that LCD has already been opened*/
    my_printf("ERROR: LCD screen has already been opened \n");
    return NULL;
  }
	
  if(!LCD_GLOBAL_INIT_FLAG ){
    lcdcInit();
    lcdcConsoleInit(&lcdStruct.console);
    LCD_GLOBAL_INIT_FLAG = TRUE;
  }
  lcdStruct.opened = TRUE;
  return &lcdStruct;
}

static int lcdrgb_close(void* structStream)
{
  int success = FALSE;
  struct LCDInfo* lcdLocalStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR: NULL pointer passed\n");
    goto EXIT_POINT;
  }
  lcdLocalStruct = (struct LCDInfo*)structStream;
  if(!lcdLocalStruct->opened){
    my_printf("ERROR: LCD screen has already been opened \n");
    goto EXIT_POINT;
  }
  lcdLocalStruct->opened = FALSE;
  success = TRUE;
 EXIT_POINT:
  return success;
}

static uint8_t lcdrgb_read(void* structStream)
{
  int success = FALSE;
  struct LCDInfo* lcdLocalStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR: NULL pointer passed\n");
    goto EXIT_POINT;
  }
  lcdLocalStruct = (struct LCDInfo*)structStream;
  if(!lcdLocalStruct->opened){
    my_printf("ERROR: LCD screen has already been opened \n");
    goto EXIT_POINT;
  }

  my_printf("ERROR: LCD screen cannot be read\n");
  success = TRUE;
 EXIT_POINT:
  return success;
}

static int lcdrgb_write(uint8_t val, void* structStream)
{
  int success = FALSE;
  struct LCDInfo* lcdLocalStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR: NULL pointer passed\n");
    goto EXIT_POINT;
  }
  lcdLocalStruct = (struct LCDInfo*)structStream;
  if(!lcdLocalStruct->opened){
    my_printf("ERROR: LCD screen has already been opened \n");
    goto EXIT_POINT;
  }

  lcdcConsolePutc(&lcdLocalStruct->console, val);
  success = TRUE;
 EXIT_POINT:
  return success;
}

static void lcdrgb_ls()
{
  my_printf ( "\nLCD RGB\n");
  my_printf(lcdRGBPrefix);
  my_printf("\n");
}


/*************************Potentiometer*************************/
char ANALOG_GLOBAL_INIT_FLAG = FALSE; // For both pot and resistance
struct PotInfo{
  unsigned int opened:1;
} potStruct = {FALSE};

static void* pot_open(const char* path)
{
  void* returnPtr = NULL;
  myAssert(prefix_string(path, potentiometerPrefix),BUG_FOUND);
  
  if (!equal_strings(potentiometerPrefix, path)) {
    my_printf("ERROR:Potentiometer path: incorrectly used \n");
    goto EXIT_POINT;
  }
  if(potStruct.opened){
    my_printf("ERROR:Potentiometer is already opened \n");
    goto EXIT_POINT;
  }
  if (!ANALOG_GLOBAL_INIT_FLAG){
    adc_init();
    ANALOG_GLOBAL_INIT_FLAG = TRUE;
  }
  potStruct.opened = TRUE;
  returnPtr = &potStruct;

 EXIT_POINT:
  return returnPtr;

}

static int pot_close(void* structStream)
{
  struct PotInfo* potInfoStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  potInfoStruct = (struct PotInfo*)structStream;

  if(!potInfoStruct->opened){
    my_printf("ERROR: Potentiometer has not been opened \n");
    return FALSE;
  }  
  potInfoStruct->opened = FALSE;
  return TRUE;
}

static uint8_t pot_read(void* structStream)
{
  struct PotInfo* potInfoStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  potInfoStruct = (struct PotInfo*)structStream;

  if(!potInfoStruct->opened){
    my_printf("ERROR: Potentiometer has not been opened \n");
    return FALSE;
  }  

  return adc_read(ADC_CHANNEL_POTENTIOMETER);
}

static int pot_write(uint8_t val, void* structStream)
{
  struct PotInfo* potInfoStruct = NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  potInfoStruct = (struct PotInfo*)structStream;

  if(!potInfoStruct->opened){
    my_printf("ERROR: Potentiometer has not been opened \n");
    return FALSE;
  }  
  my_printf("ERROR: Potentiometer cannot be written to \n");
  return FALSE;
}

static void pot_ls()
{
  my_printf ( "\nPotentiometer\n");
  my_printf(potentiometerPrefix);
  my_printf("\n");
}

/*************************Thermistor*************************/
struct ThermistorInfo{
  unsigned int opened:1;
} thermistorStruct = {FALSE};

static void* thermistor_open(const char* path)
{
  void* returnPtr = NULL;
  myAssert(prefix_string(path, thermistorPrefix),BUG_FOUND);
  
  if (!equal_strings(thermistorPrefix, path)) {
    my_printf("ERROR:Thermistor path: incorrectly used \n");
    goto EXIT_POINT;
  }
  if(thermistorStruct.opened){
    my_printf("ERROR:Thermistor is already opened \n");
    goto EXIT_POINT;
  }
  if (!ANALOG_GLOBAL_INIT_FLAG){
    adc_init();
    ANALOG_GLOBAL_INIT_FLAG = TRUE;
  }
  thermistorStruct.opened = TRUE;
  returnPtr = &thermistorStruct;

 EXIT_POINT:
  return returnPtr;
}

static int thermistor_close(void* structStream)
{
  struct ThermistorInfo* thermistorInfoStruct= NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  thermistorInfoStruct = (struct ThermistorInfo*)structStream;

  if(!thermistorInfoStruct->opened){
    my_printf("ERROR: Thermistor has not been opened \n");
    return FALSE;
  }  
  thermistorInfoStruct->opened = FALSE;
  return TRUE;

}

static uint8_t thermistor_read(void* structStream)
{
  struct ThermistorInfo* thermistorInfoStruct= NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  thermistorInfoStruct = (struct ThermistorInfo*)structStream;

  if(!thermistorInfoStruct->opened){
    my_printf("ERROR: Thermistor has not been opened \n");
    return FALSE;
  }  

  return adc_read(ADC_CHANNEL_TEMPERATURE_SENSOR);
}

static int thermistor_write(uint8_t val, void* structStream)
{
  struct ThermistorInfo* thermistorInfoStruct= NULL;
  if(NULL == structStream){
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  thermistorInfoStruct = (struct ThermistorInfo*)structStream;

  if(!thermistorInfoStruct->opened){
    my_printf("ERROR: Thermistor has not been opened \n");
    return FALSE;
  }  
  my_printf("ERROR: Thermistor cannot be written to \n");
  return FALSE;
}

static void thermistor_ls()
{
  my_printf ( "\nTHERMISTOR\n");
  my_printf(thermistorPrefix);
  my_printf("\n");
}

/*************************Touch sensor*************************/
#define NUM_TS 4
#define TS_INIT(sw, num) {FALSE,num}	\

char TS_GLOBAL_INIT_FLAG = FALSE;
struct TsInfo{
  unsigned int opened:1;
  unsigned int idx:3;
} tsStruct[NUM_TS] = {
  TS_INIT(touch1,0),
  TS_INIT(touch2,1),
  TS_INIT(touch3,2),
  TS_INIT(touch4,3)};
typedef struct TsInfo TsStruct;

/* void TSI_Init(void); */
/* void TSI_Calibrate(void); */
/* int electrode_in(int electrodeNumber); */

static void* touch_open(const char* path)
{
  int tsIdx = -1;
  void* returnPtr = NULL;
  myAssert(prefix_string(path, touchSensorPrefix),BUG_FOUND);
  tsIdx = getIntFromStr(&path[string_length(touchSensorPrefix)-1]);

  if (tsIdx < 0 && tsIdx >= NUM_TS) {
    my_printf_1("ERROR:TS Index exceeded with path %s \n", path);
    goto EXIT_POINT;
  }
  if(tsStruct[tsIdx].opened){
    my_printf_1("ERROR:TS Index has already been opened:  %s \n", path);
    goto EXIT_POINT;
  }  
  tsStruct[tsIdx].opened = TRUE;
  if(!TS_GLOBAL_INIT_FLAG){
    TSI_Init();
    TSI_Calibrate(); /*Not doing individual calibration separately for now*/
    TS_GLOBAL_INIT_FLAG = TRUE;
  } 
  returnPtr = &(tsStruct[tsIdx]);

 EXIT_POINT:
  return returnPtr;
}

static int touch_close(void* structStream)
{
  TsStruct* tsStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  tsStruct = (TsStruct*)structStream;

  if (tsStruct->idx < 0 && tsStruct->idx >= NUM_TS) {
    my_printf_1int("ERROR:TS Index exceeded with path %d \n", 
		   tsStruct->idx);
    return FALSE;
  }

  if(!tsStruct->opened){
    my_printf_1int("ERROR:TS Index has not been opened:  %d \n", 
		   tsStruct->idx);
    return FALSE;
  }  
  tsStruct->opened = FALSE;

  return TRUE;
}

static uint8_t touch_read(void* structStream)
{
  TsStruct* tsStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  tsStruct = (TsStruct*)structStream;

  if (tsStruct->idx < 0 && tsStruct->idx >= NUM_TS) {
    my_printf_1int("ERROR:TS Index exceeded with path %d \n", 
		   tsStruct->idx);
    return FALSE;
  }

  if(!tsStruct->opened){
    my_printf_1int("ERROR:TS Index has not been opened:  %d \n", 
		   tsStruct->idx);
    return FALSE;
  }  

  return electrode_in(tsStruct->idx);
}

static int touch_write(uint8_t val, void* structStream)
{
  TsStruct* tsStruct= NULL;
  if(NULL == structStream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  tsStruct = (TsStruct*)structStream;

  if (tsStruct->idx < 0 && tsStruct->idx >= NUM_TS) {
    my_printf_1int("ERROR:TS Index exceeded with path %d\n", 
		   tsStruct->idx);
    (void)val;

    return FALSE;
  }
  if(!tsStruct->opened){
    my_printf_1int("ERROR:TS Index has not been opened:  %d\n" ,tsStruct->idx);
    return FALSE;
  }  

  my_printf("ERROR:TS is read only\n");
  
  return FALSE;

}

static void touch_ls()
{
  int cnt = 0;
  my_printf ( "\nTSES\n");

  for(; cnt< NUM_TS; cnt++){
    sprintf(text_buffer,"%s%d\n", touchSensorPrefix, tsStruct[cnt].idx);
    my_printf(text_buffer);
  }
}

/*********************EXTERNAL WRAPPER**********************/

struct Device{
  int majorId;
  void* minorStruct;
  /* int pid; */
  struct Device* next;
};
typedef struct Device DeviceStruct; 

struct {
  DeviceStruct* device;
}DeviceList;

/*Maintain a linked list representation of Device opaque struct 
  like you already do for FileStruct*/
DeviceStruct* create_device(const char* path)
{
  DeviceStruct* newDevice = myMalloc(sizeof(DeviceStruct));  
  newDevice->majorId = majorId_from_path(path);
  newDevice->next = NULL;
  return newDevice;
}

void free_device(DeviceStruct* deviceStruct)
{
  myFree(deviceStruct);
}

void add_device(DeviceStruct* device)
{
  myAssert(device != NULL, NULL_POINTER);
  if (NULL == DeviceList.device) {
    DeviceList.device = device;
  } else {
    DeviceStruct* currDevice = DeviceList.device;
    while(currDevice->next != NULL) {
      currDevice = currDevice->next;
    }
    currDevice->next = device;
  }

}

DeviceStruct* removeDevice(DeviceStruct* device)
{
  DeviceStruct* currDevice = NULL;
  DeviceStruct* prevDevice = NULL;
  myAssert(NULL != DeviceList.device, NULL_POINTER);

  currDevice = DeviceList.device;
  while(NULL != currDevice &&
	currDevice != device){
    prevDevice = currDevice;
    currDevice = currDevice->next;
  }

  myAssert(NULL != currDevice, DEVICE_NOT_FOUND);
  if(NULL == prevDevice){
    DeviceList.device = currDevice->next;
  } else {
    prevDevice = currDevice->next;
  }
  currDevice->next = NULL;

  return currDevice;
}

/******************EXTERNAL INTERFACE**************************/
inline DeviceStruct* checkedDeviceStructCast(void* stream)
{
  DeviceStruct* tmp = NULL;
  if(NULL == stream) {
    my_printf("ERROR:NULL pointer passed\n");
    return FALSE;
  }
  tmp = (DeviceStruct*)stream;
  if(tmp->majorId> NUM_DEVICES ||tmp->majorId < 0) {
    sprintf(text_buffer, "ERROR: Device ID %d is invalid\n", tmp->majorId);
    my_printf(text_buffer);
    tmp = NULL;
  }
  return tmp;
}

/*Device struct is only for referencing data during the time the file 
  is opened for operation. Its a reference from caller side.
  The file system is persistent and independent of deviceStruct 
  life time
*/
void* my_fopen(const char* path)
{
  void* minorStruct = deviceTypes[majorId_from_path(path)].open(path);

  if(minorStruct){
    DeviceStruct* deviceRef = create_device(path);
    add_device(deviceRef);
    deviceRef->minorStruct = minorStruct;
    return deviceRef;
  }else{
    return NULL;
  }
}

void my_fclose(void* stream)
{
  DeviceStruct* tmp = checkedDeviceStructCast(stream);

  if(tmp){
    deviceTypes[tmp->majorId].close(tmp->minorStruct);
    removeDevice(tmp);
    free_device(tmp);
  }else {
    my_printf("ERROR:Device not closed as not found\n");
  }
}

void my_fputc(const char ch, void* stream)
{
  DeviceStruct* tmp = checkedDeviceStructCast(stream);
  if(tmp)
    deviceTypes[tmp->majorId].write(ch, tmp->minorStruct);
  else 
    my_printf("ERROR:Device not found\n");
}

char my_fgetc(void* stream)
{
  DeviceStruct* tmp = checkedDeviceStructCast(stream);
  if(tmp)
    return deviceTypes[tmp->majorId].read(tmp->minorStruct);
  else
    return 0;
}

void my_fcreate(const char* path, int isDirectory)
{
  if(majorId_from_path(path) == FILES)
    file_create(path, isDirectory);
  else
    my_printf_1("ERROR: Invalid system path %s \n",path);
}

void my_fdelete(const char* path)
{
  if(majorId_from_path(path) == FILES)
    file_delete(path);
  else
    my_printf_1("ERROR: Invalid system path %s\n", path);
}

/*Display all files - hw and sw here*/
void my_ls()
{
  int cnt = 0;
  for (;cnt<NUM_DEVICES; ++cnt){
    deviceTypes[cnt].ls();
  }
}
/**********************HARDWARE API******************************/
void setLedValues(int* ledValues)
{
	int tmp = 0; void* file = NULL;
	for(; tmp < NUM_LED; tmp++){
		sprintf(text_buffer, "%s%d", ledPathPrefix, tmp);
		file = my_fopen(text_buffer);
		my_fputc((const char)(ledValues[tmp]), file);
		my_fclose(file);
	}
}
void getSwitchValues(int* switchValues)
{
	int idx = 0; void* file = NULL;
	for (; idx < NUM_SWITCH; idx++){
		sprintf(text_buffer, "%s%d", switchPathPrefix, idx);
		file = my_fopen(text_buffer);
		switchValues[idx] = my_fgetc(file);
		my_fclose(file);
	}
}

void setLCDDisplay(const char ch)
{
	void* file = my_fopen(lcdRGBPrefix);
	my_fputc(ch, file);
	my_fclose(file);
}

int getPotentiometerValue()
{
	void* file = my_fopen(potentiometerPrefix);
	char ch = my_fgetc(file);
	my_fclose(file);
	
	return ch;
}

int getThermistorValue()
{
	void* file = my_fopen(thermistorPrefix);
	char ch = my_fgetc(file);
	my_fclose(file);
	
	return ch;
}

void getTouchSensorValue(int* touchSensorArray)
{
	void* file = NULL; int idx = 0;
	for(;idx < NUM_TS; idx++){
		sprintf(text_buffer, "%s%d", touchSensorPrefix, idx);
		file = my_fopen(text_buffer);
		touchSensorArray[idx]= my_fgetc(file);
		my_fclose(file);
	}	
}

/*********************TEST CODE**********************/
/* int main() */
/* { */
/*   const char* str = "/dir"; */
/*   const char* str2 = "/dir/file1"; */
/*   const char* str3 = "/file1"; */
/*   const char* str4 = "/dir/fil4"; */

/*   printf("\n"); */
/*   my_ls(); */
/*   my_fcreate(str, TRUE); */
/*   my_fcreate(str2, FALSE); */
/*   my_fcreate(str3, FALSE); */
/*   my_fcreate(str4, FALSE); */

/*   /\*negative test conditions*\/ */
/*   my_fcreate(str, FALSE); */
/*   my_fcreate(str2, TRUE);/\*directory and file of same full path not allowed*\/ */
/*   my_fcreate(str2, FALSE); */

/*   printf("\n"); */
/*   my_ls(); */
/*   { */
/*     void* stream2 = my_fopen(str); /\*should return null pointer*\/ */
/*     void* stream = my_fopen(str2); */

/*     my_fputc('h',stream); */
/*     my_fputc('e',stream); */
/*     my_fputc('l',stream); */
/*     my_fputc('p',stream); */

/*     { */
/*       int cnt = 0; */
/*       for(; cnt<7;cnt++){ */
/*   	char ch = my_fgetc(stream); */
/*   	printf ( "First character:%c \n",ch); */
/*       } */
/*     } */
/*     my_fclose(stream); */
/*     my_fclose(stream2);/\*NULL pointer passed in*\/ */
/*   } */

/*   { */
/*      /\*LED1*\/ */
/*     const char* str = "/dev/hw/led0"; */
/*     void* stream = NULL; */
/*     my_ls(); */
/*     my_fcreate(str, TRUE); */
/*     my_ls(); */

/*     stream = my_fopen(str); */
/*     my_fputc(0,stream); */
/*     my_fputc(1,stream); */
/*     my_fclose(stream); */

/*     my_fdelete(str); */
/*     my_ls(); */
/*   } */

/*   { */
/*     const char* str = "/dev/hw/sw1"; */
/*     void* stream = NULL; */

/*     my_fcreate(str, TRUE); */
/*     my_ls(); */

/*     stream = my_fopen(str); */
/*     { */
/*       char ch  = my_fgetc(stream); */
/*       printf("\n %d \n",ch); */
/*     } */
/*     my_fclose(stream); */
/*     my_fdelete(str); */
/*   } */

/*   printf("File deletion \n"); */
/*   my_fdelete(str);/\*negative test point*\/ */
/*   my_ls(); */


/*   my_fdelete(str3); */
/*   my_ls(); */

/*   /\* my_fdelete(str2); *\/ */
/*   /\* my_ls(); *\/ */


/*   /\* my_fdelete(str); *\/ */
/*   /\* my_ls(); *\/ */

/*   return 0; */
/* } */
