/*
 * buttons_my.c
 *
 *  Created on: Dec 16, 2025
 *      Author: urijkazak
 */


#include "buttons_my.h"
#include "main.h"

static button_t btn_s1;
static button_t btn_s2;

static inline uint8_t btn_read_pressed(const btn_hw_t* hw)
{
	// Vrací 1 když je tlačítko stisknuté
	GPIO_PinState s = HAL_GPIO_ReadPin(hw->port, hw->pin);
	uint8_t level = (s == GPIO_PIN_SET) ? 1u : 0u;
	return (level == hw->active_level) ? 1u : 0u;
}

static inline bool take_flag(volatile uint8_t* f)
{
	if (*f) { *f = 0; return true; }
	return false;
}

void btn_init(button_t* b, const btn_hw_t* hw)
{
	b->hw = *hw;


	uint32_t now = HAL_GetTick();
	b->raw = btn_read_pressed(&b->hw);
	b->stable = b->raw;
	b->raw_change_ms = now;

	b->press_ms = 0;
	b->long_sent = 0;
	b->repeat_ms = 0;

	b->evt_press = 0;
	b->evt_release = 0;
	b->evt_long = 0;
	b->evt_repeat = 0;


}

void buttons_board_init(void)
{
	// GPIO je už inicializované v MX_GPIO_Init()

	// PULL-UP => stisk = RESET
	const btn_hw_t hwS1 = {
			.port = S1_GPIO_Port,
			.pin  = S1_Pin,
			.active_level = 0,
			.debounce_ms = 50,
			.long_ms = 800,
			.repeat_start_ms = 800,
			.repeat_period_ms = 120
	};

	const btn_hw_t hwS2 = {
			.port = S2_GPIO_Port,
			.pin  = S2_Pin,
			.active_level = 0,
			.debounce_ms = 50,
			.long_ms = 800,
			.repeat_start_ms = 800,
			.repeat_period_ms = 120
	};

	btn_init(&btn_s1, &hwS1);
	btn_init(&btn_s2, &hwS2);

}


void btn_task(button_t* b, uint32_t now_ms)
{
	// 1) Polling: aktualizace raw
	uint8_t r = btn_read_pressed(&b->hw);
	if (r != b->raw) {
		b->raw = r;
		b->raw_change_ms = now_ms;
	}


	// 2) Debounce: pokud raw drží debounce_ms, přepni stable
	if (b->stable != b->raw) {
		if ((uint32_t)(now_ms - b->raw_change_ms) >= b->hw.debounce_ms) {
			b->stable = b->raw;

			if (b->stable) {
				// potvrzený stisk
				b->evt_press = 1;
				b->press_ms = now_ms;
				b->long_sent = 0;
				b->repeat_ms = now_ms;
			} else {
				// potvrzené uvolnění
				b->evt_release = 1;
			}
		}
	}

	// 3) LONG / REPEAT jen při držení
	if (b->stable) {
		uint32_t held = (uint32_t)(now_ms - b->press_ms);

		// LONG (jen jednou)
		if (!b->long_sent && held >= b->hw.long_ms) {
			b->evt_long = 1;
			b->long_sent = 1;
		}

		// REPEAT (periodicky)
		if (b->hw.repeat_period_ms > 0) {
			if (held >= b->hw.repeat_start_ms) {
				if ((uint32_t)(now_ms - b->repeat_ms) >= b->hw.repeat_period_ms) {
					b->evt_repeat = 1;
					b->repeat_ms = now_ms;
				}
			}
		}
	}


}

void buttons_task(uint32_t now_ms)
{
	btn_task(&btn_s1, now_ms);
	btn_task(&btn_s2, now_ms);
}

button_t* button_S1(void) { return &btn_s1; }
button_t* button_S2(void) { return &btn_s2; }

bool btn_pressed(button_t* b)  { return take_flag(&b->evt_press); }
bool btn_released(button_t* b) { return take_flag(&b->evt_release); }
bool btn_long(button_t* b)     { return take_flag(&b->evt_long); }
bool btn_repeat(button_t* b)   { return take_flag(&b->evt_repeat); }

bool btn_is_down(button_t* b)  { return (b->stable != 0); }

