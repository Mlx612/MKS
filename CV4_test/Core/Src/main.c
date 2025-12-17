/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sct.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_Q 12

/* Temperature sensor calibration value address */
#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
/* Internal voltage reference calibration value address */
#define VREFINT_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7BA))


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static volatile uint32_t raw_pot=0;



// ===== 7SEG segment masks (bit mapping from your sct.c tables) =====
// Segment names: A(top), B(top-right), C(bottom-right), D(bottom), E(bottom-left), F(top-left), G(middle)
// DP = decimal point

// DIS1 (hundreds) is shifted by +16 in reg_values[0]
#define D1_A   (1u << (16u+1u))
#define D1_B   (1u << (16u+0u))
#define D1_C   (1u << (16u+14u))
#define D1_D   (1u << (16u+13u))
#define D1_E   (1u << (16u+12u))
#define D1_F   (1u << (16u+2u))
#define D1_G   (1u << (16u+3u))
#define D1_DP  (1u << (16u+15u))

// DIS2 (tens) is reg_values[1] (no <<16). Order: ----PCDEGFAB----
#define D2_A   (1u << (5u))
#define D2_B   (1u << (4u))
#define D2_C   (1u << (10u))
#define D2_D   (1u << (9u))
#define D2_E   (1u << (8u))
#define D2_F   (1u << (6u))
#define D2_G   (1u << (7u))
#define D2_DP  (1u << (11u))   // potvrzené i v reg_values[4]

// DIS3 (ones) is reg_values[2] (no shift). Same as DIS1 but without +16
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
    case 0: // DIS1
        switch (seg) {
        case 0: return D1_A; case 1: return D1_B; case 2: return D1_C; case 3: return D1_D;
        case 4: return D1_E; case 5: return D1_F; case 6: return D1_G; default: return D1_DP;
        }
    case 1: // DIS2
        switch (seg) {
        case 0: return D2_A; case 1: return D2_B; case 2: return D2_C; case 3: return D2_D;
        case 4: return D2_E; case 5: return D2_F; case 6: return D2_G; default: return D2_DP;
        }
    default: // DIS3
        switch (seg) {
        case 0: return D3_A; case 1: return D3_B; case 2: return D3_C; case 3: return D3_D;
        case 4: return D3_E; case 5: return D3_F; case 6: return D3_G; default: return D3_DP;
        }
    }
}


typedef struct {
    uint8_t digit; // 0=DIS1,1=DIS2,2=DIS3
    uint8_t seg;   // 0=A,1=B,2=C,3=D,4=E,5=F,6=G,7=DP
} node_t;

// ===== GLOBAL "around the whole 3-digit display" path =====
// perimeter: top across 3 digits, right side, bottom across 3 digits, left side
// + DP across (so "vč. desetinných teček")
static const node_t path[] = {
    // TOP
    {0,0}, {1,0}, {2,0},

    // RIGHT side (rightmost digit)
    {2,1}, {2,2},

    // BOTTOM + dots
    {2,7}, {2,3},   // DIS3: DP then D (vole nemenit je to spravne !!!!!!!!!)
	{1,7}, {1,3},   // DIS2: DP then D
	{0,7}, {0,3},   // DIS1: Dp then D

    // LEFT side (leftmost digit)
    {0,4}, {0,5},
};
#define PATH_N (sizeof(path)/sizeof(path[0]))

static int path_dir = +1;      // +1 dopředu, -1 zpět
static uint8_t path_i = 0;     // index v path
static uint32_t next_step_ms = 0;


// define void function;
void update_view(void);



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	//	uint32_t r_val=0;


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */
	sct_init();
	HAL_ADCEx_Calibration_Start(&hadc);
	HAL_ADC_Start_IT(&hadc);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{


		uint32_t now = HAL_GetTick();

		// S1: smer dopredu, S2: smer zpet (pull-up => stisk = 0)
		if (HAL_GPIO_ReadPin(S1_GPIO_Port, S1_Pin) == GPIO_PIN_RESET) {
		    path_dir = +1;
		}
		if (HAL_GPIO_ReadPin(S2_GPIO_Port, S2_Pin) == GPIO_PIN_RESET) {
		    path_dir = -1;
		}


		// rychlost 20..300 ms / segment
		uint32_t step_ms = 20u + (raw_pot * (300u - 20u)) / 4095u;

		if ((int32_t)(now - next_step_ms) >= 0) {
		    next_step_ms = now + step_ms;

		    // posun po společné trase
		    if (path_dir > 0) {
		        path_i++;
		        if (path_i >= PATH_N) path_i = 0;
		    } else {
		        if (path_i == 0) path_i = (uint8_t)(PATH_N - 1);
		        else path_i--;
		    }

		    uint32_t mask = seg_mask(path[path_i].digit, path[path_i].seg);
		    sct_led(mask);
		}

		HAL_Delay(1);



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SCT_NOE_Pin|SCT_CLK_Pin|SCT_SDI_Pin|SCT_NLA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : S2_Pin S1_Pin */
  GPIO_InitStruct.Pin = S2_Pin|S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SCT_NOE_Pin SCT_CLK_Pin SCT_SDI_Pin SCT_NLA_Pin */
  GPIO_InitStruct.Pin = SCT_NOE_Pin|SCT_CLK_Pin|SCT_SDI_Pin|SCT_NLA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	uint32_t s = HAL_ADC_GetValue(hadc);
	static  uint32_t avg_pot = 0;

		if (avg_pot == 0) avg_pot = s << ADC_Q;  // быстрая инициализация
		avg_pot -= (avg_pot >> ADC_Q);
		avg_pot += s;
		raw_pot = (avg_pot >> ADC_Q);
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
