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
#include "dma.h"
#include "fatfs.h"
#include "rtc.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"
#include "stm32f4xx_hal.h"

#include <stdio.h>    
#include <stdlib.h>  
#include <string.h>     
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

char str1[60];  // Переменная для вывода через UART
uint16_t raw_temper;
float temper;
uint8_t dt[8];

extern RTC_HandleTypeDef hrtc;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern char SDPath[4];

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint8_t status;
	char c;
	
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
  MX_DMA_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
	
	// SD card
	
	FATFS fileSystem;
  FIL testFile;
  uint8_t testBuffer[16] = "SD write success";
  UINT testBytes;
  FRESULT res;
	
	/*
	/// тестовая запись на sd
	
  if(f_mount(&fileSystem, SDPath, 1) == FR_OK)
  {
    uint8_t path[13] = "tess.txt";
    path[12] = '\0';
    res = f_open(&testFile, (char*)path, FA_WRITE | FA_CREATE_ALWAYS);
    res = f_write(&testFile, testBuffer, 16, &testBytes);
    res = f_close(&testFile);
  }
	*/
	
	// ds18b20 датчик температуры
	
	port_init();
	status = ds18b20_init(SKIP_ROM);
	sprintf(str1,"Init Status: %d\r\n",status);
	HAL_UART_Transmit(&huart1,(uint8_t*)str1,strlen(str1),0x1000);
	HAL_TIM_Base_Start(&htim4);
	
	
	// Дата и время
		
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	sTime.Hours   = 12;
	sTime.Minutes = 30;
	sTime.Seconds = 0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	sDate.WeekDay = RTC_WEEKDAY_THURSDAY;
	sDate.Month   = RTC_MONTH_SEPTEMBER;
	sDate.Date    = 4;
	sDate.Year    = 25; 

	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		//�?змерение температуры
		
		ds18b20_MeasureTemperCmd(SKIP_ROM, 0);
		HAL_Delay(800);
		ds18b20_ReadStratcpad(SKIP_ROM, dt, 0);
		sprintf(str1,"STRATHPAD: %02X %02X %02X %02X %02X %02X %02X %02X; ",
		dt[0], dt[1], dt[2], dt[3], dt[4], dt[5], dt[6], dt[7]);
		HAL_UART_Transmit(&huart1,(uint8_t*)str1,strlen(str1),0x1000);
		raw_temper = ((uint16_t)dt[1]<<8)|dt[0];
		//if(ds18b20_GetSign(raw_temper)) c='-';
		//else c='+';
		char sign = ds18b20_GetSign(raw_temper) ? '-' : '+';
		temper = ds18b20_Convert(raw_temper);
		//sprintf(str1,"Raw t: 0x%04X; t: %c%.2f\r\n", raw_temper, c, temper);
		HAL_UART_Transmit(&huart1,(uint8_t*)str1,strlen(str1),0x1000);		
		
		// Запись на sd card

		FRESULT res;
		res = f_mount(&fileSystem, SDPath, 1);
		if (res == FR_OK) {
				FIL testFile;
				res = f_open(&testFile, "temp_log.txt", FA_WRITE | FA_OPEN_APPEND);
				if (res == FR_OK) {

						RTC_TimeTypeDef sTime = {0};
						RTC_DateTypeDef sDate = {0};
						
						if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
								HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) {
		
								int16_t whole = (int16_t)temper;
								int16_t frac = (int16_t)((temper - whole) * (temper >= 0 ? 100 : -100) + 0.5f);
								if (frac >= 100) {
										frac = 0;
										whole += (sign == '+') ? 1 : -1;
								}

								// дата время температура
								f_printf(&testFile, "[%02d-%02d-%02d %02d:%02d:%02d] %c%d.%02d C\r\n",
												 2000 + sDate.Year, sDate.Month, sDate.Date,
												 sTime.Hours, sTime.Minutes, sTime.Seconds,
												 sign, whole, frac);

						} else {
								// Если ошибка RTC - без времени
								f_printf(&testFile, "[--:--:--] %c%d.%02d C\r\n", sign, (int16_t)temper, 0);
						}

						f_sync(&testFile);
						f_close(&testFile);
						HAL_UART_Transmit(&huart1, (uint8_t*)"Saved to SD\r\n", 13, 1000);

				} else {
						sprintf(str1, "f_open failed: %d\r\n", res);
						HAL_UART_Transmit(&huart1, (uint8_t*)str1, strlen(str1), 1000);
				}
		} else {
				sprintf(str1, "f_mount failed: %d\r\n", res);
				HAL_UART_Transmit(&huart1, (uint8_t*)str1, strlen(str1), 1000);
		}
		
    HAL_Delay(1000);  // Каждые 5 секунды (3700)
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
