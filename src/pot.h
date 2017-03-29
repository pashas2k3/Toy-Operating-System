/*
 * pot.h
 *
 *  Created on: Nov 1, 2015
 *      Author: Vikram
 */

#ifndef POT_H_
#define POT_H_

#define ADC_CHANNEL_POTENTIOMETER   	0x14
#define ADC_CHANNEL_TEMPERATURE_SENSOR  0x1A

void adc_init(void);
unsigned int adc_read(uint8_t channel) ;

#endif /* POT_H_ */
