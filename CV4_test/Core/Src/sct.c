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

static const uint32_t reg_values[5][10] = {
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

	    /* [3] LED bargraf 0..8 — биты 4..11: //----43215678---- */
	    {
	    	      0u,                                                   // 0
	    	      ((1u<< (16+8))),                                      // 1: 4
	    	      ((1u<< (16+8))|(1u<< (16+9))),                        // 2: 4,3
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))),          // 3: 4,3,2
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))|(1u<< (16+11))), // 4: 4,3,2,1
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))|(1u<< (16+11))|(1u<< (16+7))),      // 5: +5
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))|(1u<< (16+11))|(1u<< (16+7))|(1u<< (16+6))), // 6: +6
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))|(1u<< (16+11))|(1u<< (16+7))|(1u<< (16+6))|(1u<< (16+5))), // 7: +7
	    	      ((1u<< (16+8))|(1u<< (16+9))|(1u<< (16+10))|(1u<< (16+11))|(1u<< (16+7))|(1u<< (16+6))|(1u<< (16+5))|(1u<< (16+4))), // 8: +8
	    	      0u
	    },

	    /* [4] */
	    {
	    	      0u,                                                   // 0
	    	      ((1u<< (16+15))),                      // DIS 1 dot
				  ((1u<< (11))),                         // DIS 2 dot
				  ((1u<< (15))),                         // DIS 3 dot
	    	      0u
	    },

};

void sct_value(uint16_t value, uint8_t led, uint8_t dot){
	//	ochrana proti prekoroceni max hodnoty
    if (value > 999){ value = 999;}
    if (led > 8) led = 8;
	uint8_t hundreds= (value/100)%10;
	uint8_t tens= (value/10)%10;
	uint8_t ones= value%10;

	uint32_t reg = 0;
	reg |= reg_values[0][hundreds];
	reg |= reg_values[1][tens];
	reg |= reg_values[2][ones];
	reg |= reg_values[3][led];
	reg |= reg_values[4][dot];
	sct_led(reg);

}

// ====================== 7SEG runner (orbit over 3 digits) ======================

// DIS1 (hundreds) shifted by +16 in reg_values[0]
#define D1_A   (1u << (16u+1u))
#define D1_B   (1u << (16u+0u))
#define D1_C   (1u << (16u+14u))
#define D1_D   (1u << (16u+13u))
#define D1_E   (1u << (16u+12u))
#define D1_F   (1u << (16u+2u))
#define D1_G   (1u << (16u+3u))
#define D1_DP  (1u << (16u+15u))

// DIS2 (tens) ----PCDEGFAB----
#define D2_A   (1u << (5u))
#define D2_B   (1u << (4u))
#define D2_C   (1u << (10u))
#define D2_D   (1u << (9u))
#define D2_E   (1u << (8u))
#define D2_F   (1u << (6u))
#define D2_G   (1u << (7u))
#define D2_DP  (1u << (11u))

// DIS3 (ones) same as DIS1 but without +16
#define D3_A   (1u << (1u))
#define D3_B   (1u << (0u))
#define D3_C   (1u << (14u))
#define D3_D   (1u << (13u))
#define D3_E   (1u << (12u))
#define D3_F   (1u << (2u))
#define D3_G   (1u << (3u))
#define D3_DP  (1u << (15u))

static uint32_t seg_mask(uint8_t digit, uint8_t seg)
{
    // digit: 0=DIS1, 1=DIS2, 2=DIS3
    // seg: 0=A,1=B,2=C,3=D,4=E,5=F,6=G,7=DP
    switch (digit) {
    case 0:
        switch (seg) {
        case 0: return D1_A; case 1: return D1_B; case 2: return D1_C; case 3: return D1_D;
        case 4: return D1_E; case 5: return D1_F; case 6: return D1_G; default: return D1_DP;
        }
    case 1:
        switch (seg) {
        case 0: return D2_A; case 1: return D2_B; case 2: return D2_C; case 3: return D2_D;
        case 4: return D2_E; case 5: return D2_F; case 6: return D2_G; default: return D2_DP;
        }
    default:
        switch (seg) {
        case 0: return D3_A; case 1: return D3_B; case 2: return D3_C; case 3: return D3_D;
        case 4: return D3_E; case 5: return D3_F; case 6: return D3_G; default: return D3_DP;
        }
    }
}

typedef struct {
    uint8_t digit;
    uint8_t seg;
} node_t;

// порядок DP перед D
static const node_t path[] = {
    // TOP
    {0,0}, {1,0}, {2,0},

    // RIGHT
    {2,1}, {2,2},

    // BOTTOM + dots (DP then D)
    {2,7}, {2,3},
    {1,7}, {1,3},
    {0,7}, {0,3},

    // LEFT
    {0,4}, {0,5},
};

#define PATH_N (sizeof(path)/sizeof(path[0]))

static int8_t s_dir = +1;
static uint8_t s_i = 0;
static uint32_t s_next_ms = 0;

void sct_runner_reset(void)
{
    s_dir = +1;
    s_i = 0;
    s_next_ms = 0;
    sct_led(seg_mask(path[s_i].digit, path[s_i].seg));
}

void sct_runner_update(uint32_t step_ms, int dir)
{
    uint32_t now = HAL_GetTick();

    // обновляем направление (если пришло нормальное значение)
    if (dir > 0) s_dir = +1;
    else if (dir < 0) s_dir = -1;

    if ((int32_t)(now - s_next_ms) < 0) return;
    s_next_ms = now + step_ms;

    // шаг по path
    if (s_dir > 0) {
        s_i++;
        if (s_i >= PATH_N) s_i = 0;
    } else {
        if (s_i == 0) s_i = (uint8_t)(PATH_N - 1);
        else s_i--;
    }

    sct_led(seg_mask(path[s_i].digit, path[s_i].seg));
}

















