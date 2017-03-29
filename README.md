Documentation for System Programmer

1. Architecture

Shell works in the unprivileged mode. Therefore to access I/O devices, it
needs to go through SVC.

The user calls the SVC wrapper method which in turn calls the SVCHandler
via an interrupt. This will then find the SVCHandler in the interrupt
vector table and call the SVCHandler with the ID passed from the wrapper
SVC calls.

When in the SVCHandler, the "switch yard" there shall identify the function to call
according to the ID and use the framePtr for getting the argument and assigning
the return values.

It has been written with the assumption, the number of arguments is <= 4 and all
arguments are representable by int

2. Adding new SVC API

  a. Write a non-interrupt API to call into for accessing the I/O device in privileged mode
  b. Add the enum ID for SVC routine to SVCId in svc.h
  c. Add the wrapper SVC method called by user space to svc.c and svc.h
  d. Add the call to switch structure in SVCHandlerInC in svc.c

3. APIs available

All SVC APIs begin with a prefix of SVC. Some of the specific APIs available
are as follows

    a. Memory Management
       i.  SVCMalloc
       ii. SVCFree
    b. File Management
       i.  SVCCreate
       ii. SVCDelete
       iii.SVCFPutC
       iv. SVCFGetC
       v.  SVCFOpen
       vi. SVCFClose
    c. Device Management
       i.  SVCLedSignal
       ii. SVCSwitchIn
       iii.SVCLCDDisplay
       iv. SVCPotentiometerIn
       v.  SVCThermistorIn
       vi. SVCTouchSensorIn
    d. UART I/O
       i. SVCUartIn
       ii.SVCUartDisp

