#include "memory_manager.h"
#include "utility.h"
#include "sdram.h"
/**********UTILITIES**********/

#define FOREACH_ERROR_CONDN(ERROR_CONDN) \
        ERROR_CONDN(SUCCESS)   \
	ERROR_CONDN(DOUBLE_FREE)\
	ERROR_CONDN(OUT_OF_MEMORY)\
	ERROR_CONDN(UNIMPLEMENTED)\
	ERROR_CONDN(INVALID_PID_ACCESS)\
	ERROR_CONDN(NO_ALLOCATED_MEMORY)\
	ERROR_CONDN(ASSERTION_FAILED)\
	ERROR_CONDN(NULL_POINTER)\
	ERROR_CONDN(UNALLOCATED_MEMORY)\
	ERROR_CONDN(MEMORY_CORRUPTION)\
	ERROR_CONDN(HEADER_CORRUPTION)\

#define GENERATE_ENUM(ENUM) ENUM,

#define GENERATE_STRING(STRING) #STRING,

enum ERROR_CONDN_ENUM {
    FOREACH_ERROR_CONDN(GENERATE_ENUM)
};

static const char *ERROR_CONDN_STRING[] = {
    FOREACH_ERROR_CONDN(GENERATE_STRING)
};

static void fatal_error(int error_enum)
{
  if (error_enum>SUCCESS){
    my_printf_1( "FATAL ERROR: %s\n", ERROR_CONDN_STRING[error_enum]);
  }
  exit(1);
}

static void myAssert(int condn)
{
  if(!condn){
    fatal_error(ASSERTION_FAILED);
  }
}

static int process_error(int error_enum)
{
  if (error_enum>SUCCESS){
    my_printf_1( "FATAL ERROR: %s\n",
	    ERROR_CONDN_STRING[error_enum]);
  }
  return error_enum;
}

/****************DATA STRUCTURES**********************/
/*
  Memory Header (for free and allocated chunks) -
     1 byte of free flag + PID
     1 byte for number of allocated/free bytes

*/
struct MemoryHeader{
  uint8_t memoryFlag:1;
  uint8_t PID:7;
  uint32_t memoryChunkBytes;
};
typedef struct MemoryHeader MemHeader;

/************PID SPECIFIC CODE**********************/
const uint8_t MAX_NUM_PID= 128;
static int getCurrentPID()
{
  int currentPID = 0;
  myAssert(currentPID < MAX_NUM_PID);
  return currentPID;
}

static int isInPIDMemory(MemHeader* headerRef)
{
  return (getCurrentPID() == (headerRef->PID));
}

/************MEMORY BOOKKEEPING*****************/
/*ASSUMPTION - the logic to allocate shall make sure the pointers
  in the linked list are in ascending order*/
 
const uint32_t TOTAL_MEMORY_AVAILABLE = SDRAM_SIZE;
const uint32_t ALLOCATED_MEMORY_BYTES = SDRAM_SIZE - LCDC_FRAME_BUFFER_SIZE;
const uint32_t MEM_HEADER_BYTES =  (sizeof(struct MemoryHeader)) ;
/*As uint32_t used by malloc*/

/*Forward declaration*/
/*NOTE: using bool flag to cut down rework and introducing bugs*/
static uint8_t* initializeHeader(MemHeader* headerRef,
				 const uint8_t freeMemoryFlag,
				 const uint32_t memorySize);

/*kept as uint8_t* to keep iteration and pointer arithmetic simple*/
MemHeader* gMallocMem;
const uint8_t FREE_MEM_FLAG= 0;
const uint8_t ALLOCATE_MEM_FLAG= 1; /*need better way of specifying*/
/*ASSERTS*/
const uint8_t MAX_ALIGNMENT_MEM = 7;
inline uint8_t bytesNeededForAlignment(const uint8_t* addr)
{
  /*Only the last 3 digits (in decimal) matter for divisbility by 8
   So we can ignore the overflowing parts*/
  uint8_t numBytesNeeded = (uint8_t)((uint32_t)(addr + MAX_ALIGNMENT_MEM) & 
			    ~MAX_ALIGNMENT_MEM) -  (uint8_t)addr;
  myAssert(numBytesNeeded <= MAX_ALIGNMENT_MEM) ;
  return numBytesNeeded;
}

inline MemHeader* getMemoryHead()
{
  if (NULL == gMallocMem) {
	  uint8_t* tmp = (uint8_t*)SDRAM_START + LCDC_FRAME_BUFFER_SIZE ;
    /*Since memory held by the process as long as it exists, not maintaining pointer at original level*/
    gMallocMem = (MemHeader*)(tmp + bytesNeededForAlignment(tmp));
    
    myAssert(NULL != gMallocMem);
    /*Initialize the header
     If it returns NULL, it is an assertion - malloc or free*/
    myAssert(NULL != initializeHeader(gMallocMem, FREE_MEM_FLAG,
				      ALLOCATED_MEMORY_BYTES-
				      MEM_HEADER_BYTES) );
  }
  return gMallocMem;
}

inline MemHeader* getMemoryLimit()
{
  return (MemHeader*)(((uint8_t*)getMemoryHead()) +
		      ALLOCATED_MEMORY_BYTES);
}

inline uint8_t* getMemoryChunkPtr(MemHeader* headerRef)
{
  return (uint8_t*)(&headerRef[1])/* + headerRef->alignment */;
}
inline int isMemoryFree(const MemHeader* headerRef)
{
  return (headerRef->memoryFlag == FREE_MEM_FLAG) &&
    (headerRef >= getMemoryHead()) &&
    (headerRef < (MemHeader*)( ((uint8_t*)getMemoryLimit())-
				       MEM_HEADER_BYTES - 1));
}
/*Includes header bytes*/
/* Use this one only for debugging*/
inline uint32_t getEntireMemory(MemHeader* headerRef)
{
  return /* headerRef->alignment + */ headerRef->memoryChunkBytes +
    MEM_HEADER_BYTES;
}
inline uint32_t getEntireMemoryFree(MemHeader* headerRef)
{
  myAssert(isMemoryFree(headerRef));
  return getEntireMemory(headerRef);
}
inline uint32_t getEntireMemoryAllocated(MemHeader* headerRef)
{
  myAssert(!isMemoryFree(headerRef));
  return getEntireMemory(headerRef);
}

inline MemHeader* getNextHeader(const MemHeader* headerRef)
{
  return (MemHeader*)((uint8_t*)(&headerRef[1])+
		      /* headerRef->alignment + */
		      headerRef->memoryChunkBytes);
}
static uint8_t* initializeHeader(MemHeader* headerRef,
				 const uint8_t memoryFlag,
				 uint32_t memorySize)
{
  uint32_t alignmentNeedForNextHeader = 
    bytesNeededForAlignment(getMemoryChunkPtr(headerRef) + memorySize);

  /*The raw memory head should be memory aligned*/
  myAssert(!bytesNeededForAlignment(getMemoryChunkPtr(headerRef)));
  myAssert(MEM_HEADER_BYTES%8 == 0);

  /*header needs to be in valid allocated memory space
   and needs to be used only when there is some memory to have
   header for*/
  myAssert(NULL != getMemoryHead() &&
     headerRef >= getMemoryHead() &&
     headerRef < (MemHeader*)( ((uint8_t*)getMemoryLimit())-
			       MEM_HEADER_BYTES) );

  myAssert(FREE_MEM_FLAG == memoryFlag ||
	   ALLOCATE_MEM_FLAG == memoryFlag);

  headerRef->memoryFlag = memoryFlag;

  /* Alignment shouldn't be a concern as memory chunk should
     already have alignment taken into consideration    
   */

  if (FREE_MEM_FLAG != memoryFlag){
    myAssert(getCurrentPID() <= MAX_NUM_PID);
    headerRef->PID = getCurrentPID();
    
  /*Alignment adjustment to be done only in the case of 
    allocation*/
    memorySize += alignmentNeedForNextHeader;
    myAssert(memorySize <= headerRef->memoryChunkBytes);
  } 

  myAssert(memorySize <= ALLOCATED_MEMORY_BYTES - MEM_HEADER_BYTES);

  /*NOTE
    If merging two regions, alignment should be fine as 
    memorychunks are padded to be correct and memory headers 
    sizes are divisible by 8
    
    If re-using old header regions, nothing needs to be done here
  */

  /* Next memory location should be memory aligned already*/
  myAssert(!bytesNeededForAlignment(getMemoryChunkPtr(headerRef) + 
				    memorySize));
  headerRef->memoryChunkBytes = memorySize;

  /*Assertion: Allocated memory shouldn't exceed the
    limit of memories*/
  myAssert(getNextHeader(headerRef) <= getMemoryLimit());

  /*Return the allocated/freed memory
    Right after the header spaced by alignment bytes*/
  return getMemoryChunkPtr(headerRef);
}

static int memoryChunkCanBeAllocated(MemHeader* headerRef,
				     uint32_t memSize)
{
  int canBeAllocated = FALSE;

  /* No need to worry about alignment as it is already taken care of
     by the header.
     As long as the memory fits, it can be considered aligned
   */
  if (isMemoryFree(headerRef)){
    canBeAllocated = (headerRef->memoryChunkBytes >=
		      memSize );
  }

  return canBeAllocated;
}

/*ASSERTS*/
/*return NULL in case of error recoverable from */
uint8_t* findFirstFit(uint32_t memSize)
{
  MemHeader* procMem = getMemoryHead();
  uint8_t* allocatedMemPtr = NULL;

  /* Iterate over the memory
     ASSUMPTION - the first header is always valid */

  /*NOTE - validation check of memory map can be done
    during free call only for now. Or during separate
    memory check*/

  MemHeader* currHeader = procMem;
  do{
    /*NOTE
      1. headerRef 2nd byte points to uint32_t value with num of
      bytes in this chunk
      2. MEM_HEADER_BYTES byte extra needed to point to next header
      from the current header top

      NOTE: Calculated here because this information also used for
      next hole header (if available) creation
    */

    if ( memoryChunkCanBeAllocated(currHeader, memSize) ){

      uint32_t memAvailable = currHeader->memoryChunkBytes;
      /*In case the hole left after allocation
	is too small to have a header and a byte, combine it
	into this memory.
      */
      if (memAvailable <= (memSize + MAX_ALIGNMENT_MEM
			   + MEM_HEADER_BYTES)  ){
	myAssert(memSize <= memAvailable);
	memSize = memAvailable;
      }
      allocatedMemPtr = initializeHeader(currHeader,
					 ALLOCATE_MEM_FLAG, memSize);
      /* If any memory left in lower chunk initialize the 
	 header*/
      if (memSize != memAvailable){
	uint32_t lowerHoleMemory = memAvailable -
	  (currHeader->memoryChunkBytes + MEM_HEADER_BYTES);
	/* Using memoryChunkBytes instead of memSize 
	  because that includes alignment padding */
	
	myAssert(lowerHoleMemory < memAvailable);
	initializeHeader( getNextHeader(currHeader),
			  FREE_MEM_FLAG, lowerHoleMemory);
      }
      myAssert(memSize <= memAvailable);
    }
    currHeader = getNextHeader(currHeader);

  }while(!allocatedMemPtr &&
	 (currHeader < getMemoryLimit()));

  /*If nothing was found return memory shall be NULL*/
  return allocatedMemPtr;
}

static int free_helper(void* ptr)
{
  /*Look at the memory header for number of bytes to free */
  int errorID = SUCCESS;

  /* NOTE: The finding of any contiguous free regions would have
     been easier with doubly linked list (at cost of another uint32_t)

     I chose not to use that approach because:
     1. I have to validate the pointer to be freed by iterating
     over the whole list
     2. Since it is going to embedded environment. I want
     to optimize the memory consumption for book keeping
  */

  MemHeader* procMem = getMemoryHead();
  /*These will be used for memory deallocation and merging
   contiguous memory locations*/
  MemHeader* prevHeaderRef = NULL;
  MemHeader* nextHeaderRef = NULL;
  MemHeader* headerRef = NULL;

  myAssert(NULL != ptr);

  /***********Validation check*******************/
  /* Iterate over the memory map and see if there is
     any more free
     memory contiguous to this memory

     NOTE: Do the iteration before to know which header to update
     and to avoid interrupts messing anything
     1. If it is free memory before this memory- initialize its
     header only
     2. If it is memory after this, update the current header
  */
  {
    /*These will be used for map iteration*/
    MemHeader* nextHeader = NULL;
    MemHeader* prevHeader = NULL;
    MemHeader* currHeader = procMem;

    myAssert((procMem < ptr)  && (ptr<=getMemoryLimit()));
    do{
      nextHeader = getNextHeader(currHeader);
      /*If the memory pointed by current header
	is the passed in ptr*/
      if(getMemoryChunkPtr(currHeader) == ptr) {
	prevHeaderRef = prevHeader;
	nextHeaderRef = nextHeader;
	headerRef = currHeader;
      }
      prevHeader = currHeader;
      currHeader = nextHeader;
    }while(currHeader < getMemoryLimit());
  }

  /*Invalid pointer not found allocated in the memory map*/
  if(NULL == headerRef){
    errorID = MEMORY_CORRUPTION;
    goto EXIT_POINT;
  }

  /* memory Freeing */
  {
    int prevMemoryFree = (NULL != prevHeaderRef) &&
      isMemoryFree(prevHeaderRef);
    int nextMemoryFree = (NULL != nextHeaderRef) &&
      isMemoryFree(nextHeaderRef);

    /*check prevHeader and nextHeader if they are free*/
    if(isMemoryFree(headerRef)){
      errorID = UNALLOCATED_MEMORY;
      goto EXIT_POINT;
    }
    if(!isInPIDMemory(headerRef)){
      errorID = INVALID_PID_ACCESS;
      goto EXIT_POINT;
    }

    /***********MEMORY FREEING************/
    /*
       1. Free allocated Memory
       2. Integrate neighboring contiguous free regions

       NOTE: If previous header is free, use that header for initializing and include its memory chunk
       NOTE: If next header is free (and not previous one), use the current header for initialization
     */
    {
      MemHeader* initHeaderRef = (prevMemoryFree)? prevHeaderRef:
	headerRef;

      /*memory of current chunk*/
      uint32_t memoryToFree = headerRef->memoryChunkBytes;
      if(prevMemoryFree) {
	/* memory of previous chunk if needed
	   NOTE: include the header bytes in the to be freed
	   memory for currheader*/
	memoryToFree = prevHeaderRef->memoryChunkBytes +
	  getEntireMemoryAllocated(headerRef);
      }
      if (nextMemoryFree) {
	/* memory of next chunk if needed
	   NOTE: include the header bytes in the next chunk of
	   memory */
	memoryToFree += getEntireMemoryFree(nextHeaderRef);
      }
      initializeHeader(initHeaderRef, FREE_MEM_FLAG, memoryToFree);
    }
  }

 EXIT_POINT:
  return errorID;
}
/*************DEBUGGING/PRINTING HELPERS********************/

int memoryMapDisplay(FILE* out)
{
  /*Look at the memory header for number of bytes to free */
  int errorID = SUCCESS;

  my_printf("\n**********MEMORY MAP START***************\n");
  
  /*Iterate over the map*/
  MemHeader* procMem = getMemoryHead();
  MemHeader* currHeader = procMem;
  int chunkCounter = 0;
  uint32_t netMemoryFree = 0;
  /* int alignmentCounter = 0; */
  while(currHeader < getMemoryLimit()){
    chunkCounter++;
    /* alignmentCounter += currHeader->alignment; */
    /*Print the memory header information*/
    sprintf(text_buffer,"\n Chunk %d at ptr %p", chunkCounter, currHeader);
    my_printf(text_buffer);
    
    sprintf(text_buffer, "\n Data pointer: %p",getMemoryChunkPtr(currHeader));
    my_printf(text_buffer);
    
    sprintf(text_buffer, "\n %s, PID: %d",
    		isMemoryFree(currHeader)?"Free":"Allocated",
    				(currHeader->PID) );
    my_printf(text_buffer);
    
    /* my_printf(out,"\n Alignment Bytes %d", currHeader->alignment); */
    sprintf(text_buffer,"\n Number of Bytes: %u\n",
    		currHeader->memoryChunkBytes);
    my_printf(text_buffer);
    
    /* my_printf(out,"\n Memory end: %p\n",  */
    /* 	    currHeader+   */
    /* 	    (!gisMemoryFree(currHeader))?getEntireMemoryAllocated(currHeader): getEntireMemoryFree(currHeader)); */

    netMemoryFree += currHeader->memoryChunkBytes;
    /*Move to the next header*/
    currHeader = getNextHeader(currHeader);
  }

  /*We should be pointing at the end of memory at the end
    of memory map routine*/
  if (currHeader != getMemoryLimit()) {
    errorID = MEMORY_CORRUPTION;
    goto EXIT_POINT;
  }

  /*Check memory covered with size calculated
    makes sure there is no loss of memory */
  if ( (netMemoryFree + /* alignmentCounter + */
	(MEM_HEADER_BYTES*chunkCounter)) != ALLOCATED_MEMORY_BYTES){
    errorID = MEMORY_CORRUPTION;
    goto EXIT_POINT;
  }

 EXIT_POINT:
  my_printf("\n**********MEMORY MAP END*************** \n");
  return errorID;
}

/*************EXTERNAL INTERFACE********************/

void* myMalloc(unsigned int size)
{
  void* ptr = NULL;
  if(0==size)
    goto EXIT_POINT;

  ptr = findFirstFit(size);

 EXIT_POINT:
  return ptr;
}

void myFree(void* ptr)
{
  int errorID = SUCCESS;
  if(NULL == ptr){
    errorID = NULL_POINTER;
    goto EXIT_POINT;
  }

  errorID = free_helper(ptr);

 EXIT_POINT:
  process_error(errorID);
}

void memoryMap()
{
  int errorID = memoryMapDisplay(stdout);

  process_error(errorID);
}
