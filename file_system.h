#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

void my_ls();

void* my_fopen(const char* path);
void my_fclose(void* path);
void my_fdelete(const char* path);
void my_fputc(const char ch, void* stream);
char my_fgetc(void* stream);
void my_fcreate(const char* path, int isDirectory);

/*TODO Could use bit patterns too if using array is becoming painful*/
void setLedValues(int* ledValues);
void getSwitchValues(int* switchValues);
void setLCDDisplay(const char ch);
int getPotentiometerValue();
int getThermistorValue();
void getTouchSensorValue(int* touchSensorArray);


#endif
