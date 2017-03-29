#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

inline void assert_true(condn, string)
  {
    if(!condn){
      my_fprintf(stderr,"\n ASSERTION FAILED:");
      my_fprintf(stderr,string);
      my_fprintf(stderr,"\n ");
      exit(1);
    }
  }



void testMemoryAllocation()
{
  my_fprintf(stdout,"\n---------------------------------------\n");
  my_fprintf(stdout,"--------MEMORY ALLOCATION TESTING--------\n");
  my_fprintf(stdout,"---------------------------------------\n");

  /*
     1. Test allocation of memory and deallocation for
     large memory case - 1GB memory management
     2. Test allocation of memory & deallocation and
     no allocation in other case - 30 bytes for memory management
   */
  char strBuf1[] = "Hello World!";
  char strBuf2[] = "Goodbye cruel World!";

  memoryMap();
  {
    char* str1 = (char*)myMalloc(14);
    int idx = -1;

    if(str1 != NULL){
      do{
	++idx;
	str1[idx] = strBuf1[idx];
      }while(strBuf1[idx] != '\0');

      memoryMap();
      my_fprintf(stdout, "strBuf %s\n", str1);
    } else
      my_fprintf(stdout,"++++strBuf1 not allocated+++++\n");

    {
      char* str2 = (char*)myMalloc(25);
      if(NULL != str2){
	idx = -1;
	do{
	  ++idx;
	  str2[idx] = strBuf2[idx];
	}while(strBuf2[idx] != '\0');
	memoryMap();

	my_fprintf(stdout, "strBuf2 %s\n", str2);
      } else
	my_fprintf(stdout,"++++++strBuf2 not allocated+++++\n");


      if(NULL != str1)
	myFree(str1);

      memoryMap();

      if(NULL != str2)
	myFree(str2);
    }

  }
  memoryMap();
}
void shuffle(int *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}
const unsigned int NUM_CHUNKS =10;
const unsigned int CHUNK_MEM = 5;
void testFragmentation()
{
  my_fprintf(stdout,"\n---------------------------------\n");
  my_fprintf(stdout,"--------SHUFFLE TESTING--------\n");
  my_fprintf(stdout,"---------------------------------\n");
  /* Test fragmentation with multiple and staggered
     allocation and deallocation*/
  char* strBufList[NUM_CHUNKS];
  int idx;
  int orderOfAllocation[NUM_CHUNKS];
  memoryMap();

  /*allocate the memory sequentially*/
  for (idx = 0; idx < NUM_CHUNKS ; idx++){
    strBufList[idx] = (char*)myMalloc(CHUNK_MEM*(idx+1)); /*Allocate differnt sizes*/
    my_fprintf(stdout,"Allocation chunk:%d %p\n",idx, strBufList[idx]);
    assert_true(NULL != strBufList[idx], "Shouldn't be NULL" );
    /* memoryMap(); */
  }


  /*Randomized order of freeing*/
  for (idx = 0; idx < NUM_CHUNKS ; idx++){
    orderOfAllocation[idx] = idx;
  }
  shuffle(orderOfAllocation, NUM_CHUNKS);
  for (idx = 0; idx < NUM_CHUNKS ; idx++){
    my_fprintf(stdout,"Free num%d: chunk %d %p \n",idx,
	    orderOfAllocation[idx]+1,
	    strBufList[orderOfAllocation[idx]]);
    myFree(strBufList[orderOfAllocation[idx]]);
    strBufList[orderOfAllocation[idx]] = NULL;
    /* memoryMap(); */
  }


  /*Mixed insertion and deletion - deterministically random*/
  /*Allocate larger chunks and deallocate some and insert smaller
    chunks*/
  my_fprintf(stdout,"\n---------------------------------\n");
  my_fprintf(stdout,"--------FRAGMENTATION TESTING--------\n");
  my_fprintf(stdout,"---------------------------------\n");

  for (idx = 0; idx < NUM_CHUNKS; idx++){
    /*Allocate large chunks*/
    if (idx > NUM_CHUNKS/2) {
      if(NULL != strBufList[0]){
	myFree(strBufList[0]);
	strBufList[0] = NULL;
	my_fprintf(stdout,"Chunk 0 freed \n");
      }
    }
    if(0 == idx)
      strBufList[idx] = myMalloc(CHUNK_MEM*(NUM_CHUNKS-1));
    else
      strBufList[idx] = myMalloc(CHUNK_MEM);
    my_fprintf(stdout,"Chunk %d allocated at %p\n", idx, strBufList[idx]);
  }
  memoryMap();

  for(idx = 0; idx <NUM_CHUNKS; idx++){
    if(NULL != strBufList[idx]) {
	  myFree(strBufList[idx]);
	  strBufList[idx] = NULL;
	  my_fprintf(stdout,"Chunk %d freed\n", idx);
      }
  }

  memoryMap();
}

int main(int argc, char* argv[])
{

  testMemoryAllocation();
  testFragmentation();
  return 0;
}
