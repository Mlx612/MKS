/*
 * sct.c
 *
 *  Created on: Oct 8, 2025
 *      Author: 277069
 */
#include "main.h"
#include "sct.h"

void sct_init(void){
	HAL_GPIO_WritePin(SCT_CLK_GPIO_Port, SCT_CLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SCT_NLA_GPIO_Port, SCT_NLA_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCT_NOE_GPIO_Port, SCT_NOE_Pin, GPIO_PIN_RESET);
}

void sct_led(uint32_t value){
	for(uint32_t i=0; i<32; i++){
		GPIO_PinState bit = (value & 0x1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
		HAL_GPIO_WritePin(SCT_SDI_GPIO_Port, SCT_SDI_Pin, bit);

		HAL_GPIO_WritePin(SCT_CLK_GPIO_Port, SCT_CLK_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SCT_CLK_GPIO_Port, SCT_CLK_Pin, GPIO_PIN_RESET);

		value >>=1;

	}

	HAL_GPIO_WritePin(SCT_NLA_GPIO_Port, SCT_NLA_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCT_NLA_GPIO_Port, SCT_NLA_Pin, GPIO_PIN_RESET);
}

static const uint32_t reg_values[3][10] = {
		{
				//PCDE--------GFAB @ DIS1
				0b0111000000000111 << 16,
				0b0100000000000001 << 16,
				0b0011000000001011 << 16,
				0b0110000000001011 << 16,
				0b0100000000001101 << 16,
				0b0110000000001110 << 16,
				0b0111000000001110 << 16,
				0b0100000000000011 << 16,
				0b0111000000001111 << 16,
				0b0110000000001111 << 16,
		},
		{
				//----PCDEGFAB---- @ DIS2
				0b0000011101110000 << 0,
				0b0000010000010000 << 0,
				0b0000001110110000 << 0,
				0b0000011010110000 << 0,
				0b0000010011010000 << 0,
				0b0000011011100000 << 0,
				0b0000011111100000 << 0,
				0b0000010000110000 << 0,
				0b0000011111110000 << 0,
				0b0000011011110000 << 0,
		},
		{
				//PCDE--------GFAB @ DIS3
				0b0111000000000111 << 0,
				0b0100000000000001 << 0,
				0b0011000000001011 << 0,
				0b0110000000001011 << 0,
				0b0100000000001101 << 0,
				0b0110000000001110 << 0,
				0b0111000000001110 << 0,
				0b0100000000000011 << 0,
				0b0111000000001111 << 0,
				0b0110000000001111 << 0,
		},
};

void sct_value(uint16_t value){
	//	ochrana proti prekoroceni max hodnoty
	if (value>999){
		value = 999;
	}
	uint8_t hundreds= (value/100)%10;
	uint8_t tens= (value/10)%10;
	uint8_t ones= value%10;

	uint32_t reg = 0;
	reg |= reg_values[0][hundreds];
	reg |= reg_values[1][tens];
	reg |= reg_values[2][ones];
	sct_led(reg);

}















