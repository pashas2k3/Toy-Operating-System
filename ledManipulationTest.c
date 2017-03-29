#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ledManipulation.h"
#include "utility.h"

/*******************PUSH BUTTON ROUTINES****************************************/


void switchInitClock(void)
{
  my_fprintf(stdout, "led init clock called\n");
}

void pushButtonInitClock(void)
{
  my_fprintf(stdout, "pushButton init called\n");
}

void pushbuttonInitAll(){
  my_fprintf(stdout, "pushButton init all called\n");
}

void SW1Config(void) {
  my_fprintf(stdout, "pushButton sw1 config called\n");
}

void SW2Config(void) {
  my_fprintf(stdout, "pushButton sw2 config called\n");
}

int SW1In(void) {
  my_fprintf(stdout, "pushButton sw1 in called\n");
  return TRUE;
}

int SW2In(void) {
  my_fprintf(stdout, "pushButton sw2 in called\n");
  return TRUE;
}

/******************LED ROUTINES****************************************/
void ledInitClock(void)
{
  my_fprintf(stdout, "led init clock called\n");
}

void ledInitAll(void) {
  my_fprintf(stdout, "led init all called\n");	
}

void ledOrangeConfig(void) {
  my_fprintf(stdout, "led orange config called\n");	
}

void ledYellowConfig(void) {
  my_fprintf(stdout, "led yellow config called\n");	
}

void ledGreenConfig(void) {
  my_fprintf(stdout, "led green config called\n");	
}

void ledBlueConfig(void) {
  my_fprintf(stdout, "led blue config called\n");	
}

void ledOrangeOff(void) {
  my_fprintf(stdout, "led orange off called\n");	
}

void ledYellowOff(void) {
  my_fprintf(stdout, "led yellow off called\n");	
}

void ledGreenOff(void) {
  my_fprintf(stdout, "led green off called\n");	
}

void ledBlueOff(void) {
  my_fprintf(stdout, "led blue off called\n");	
}

void ledOrangeOn(void) {
  my_fprintf(stdout, "led orange on called\n");	
}

void ledYellowOn(void) {
  my_fprintf(stdout, "led yellow on called\n");	
}

void ledGreenOn(void) {
  my_fprintf(stdout, "led green on called\n");	
}

void ledBlueOn(void) {
  my_fprintf(stdout, "led blue on called\n");	
}
