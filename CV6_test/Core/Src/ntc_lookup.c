/*
 * ntc_lookup.c
 *
 *  Created on: Oct 28, 2025
 *      Author: urijkazak
 */


#include "ntc_lookup.h"

// Таблица в формате ×10 °C (int16).
// Вставь сюда data.dlm, сгенерированный в MATLAB как целые (int16), без десятичной точки.
// Пример генерации в MATLAB: t2 = int16(round(polyval(p, ad2)*10)); dlmwrite('data.dlm', t2, 'delimiter', ',');

const int16_t ntc_lookup[1024] = {
    #include "data.dlm"
};
