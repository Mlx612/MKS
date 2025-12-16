/*
 * sct.h
 *
 *  Created on: Oct 8, 2025
 *      Author: 277069
 */

#ifndef INC_SCT_H_
#define INC_SCT_H_


#include <stdint.h>

void sct_init(void);
void sct_led(uint32_t value);
void sct_value(uint16_t value, uint8_t led, uint8_t dot);
//void sct_ledS(uint8_t led);

#endif /* INC_SCT_H_ */
