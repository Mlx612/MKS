/*
 * buttons_my.h
 *
 *  Created on: Dec 16, 2025
 *      Author: urijkazak
 */

#ifndef INC_BUTTONS_MY_H_
#define INC_BUTTONS_MY_H_



#endif /* INC_BUTTONS_MY_H_ */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// HW konfigurace tlačítka
typedef struct {
GPIO_TypeDef* port;
uint16_t      pin;


uint8_t       active_level;     // 1 = stisk = GPIO_PIN_SET, 0 = stisk = GPIO_PIN_RESET
uint16_t      debounce_ms;      // typicky 30..70 ms
uint16_t      long_ms;          // typicky 600..1200 ms
uint16_t      repeat_start_ms;  // od kdy začít opakování (např. 800 ms)
uint16_t      repeat_period_ms; // perioda opakování (např. 120 ms), 0 = vypnuto

} btn_hw_t;

// Stav tlačítka + eventy
typedef struct {
btn_hw_t hw;



uint8_t  raw;            // poslední "syrové" čtení (0/1)
uint8_t  stable;         // odfiltrovaný stabilní stav (0/1)
uint32_t raw_change_ms;  // kdy se změnil raw

uint32_t press_ms;       // čas potvrzeného stisku
uint8_t  long_sent;      // aby LONG event byl jen jednou
uint32_t repeat_ms;      // čas posledního REPEAT eventu

volatile uint8_t evt_press;
volatile uint8_t evt_release;
volatile uint8_t evt_long;
volatile uint8_t evt_repeat;


} button_t;

// Inicializace všech tlačítek na desce
void buttons_board_init(void);

// Inicializace (volat 1x po startu)
void btn_init(button_t* b, const btn_hw_t* hw);

// Periodická obsluha (volat v while(1), ideálně každých 1..10 ms)
void btn_task(button_t* b, uint32_t now_ms);

// Eventy (jednorázové — po přečtení se smažou)
bool btn_pressed(button_t* b);      // krátký stisk (PRESS)
bool btn_released(button_t* b);     // uvolnění (RELEASE)
bool btn_long(button_t* b);         // dlouhý stisk (LONG) — jen 1x
bool btn_repeat(button_t* b);       // opakování při držení (REPEAT)

// Stav (ne event) — true po celou dobu držení
bool btn_is_down(button_t* b);

void buttons_task(uint32_t now_ms);
button_t* button_S1(void);
button_t* button_S2(void);


#ifdef __cplusplus
}
#endif
