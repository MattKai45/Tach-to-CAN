/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define RPM_TIM &htim1
#define RPM_TIM_UNITS 1000 /* milliseconds */

#define NUMBER_OF_CYLINDERS 4
#define ENGINE_CYL_FACTOR (NUMBER_OF_CYLINDERS / 2)
#define RPS2RPM 60 /* Rotations per second to Rotations per minute */

/* Set the arbitration ID here */
#define ENGINE_RPM_ARB_ID    0x090

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint32_t t1 = 0;
uint32_t t2 = 0;

uint32_t engine_rpm = 0;

CAN_TxHeaderTypeDef Header = {
        .DLC                = 8,
        .ExtId              = ENGINE_RPM_ARB_ID,
        .StdId              = ENGINE_RPM_ARB_ID,
        .IDE                = CAN_ID_STD,
        .RTR                = CAN_RTR_DATA,
        .TransmitGlobalTime = DISABLE
};

uint32_t blink = 0;
#define NORMAL_OPERATION 750
#define ERROR_MODE       150
uint32_t blink_rate = NORMAL_OPERATION;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

HAL_StatusTypeDef can_filter( CAN_HandleTypeDef *pcan, uint32_t id, uint32_t mask, uint32_t format, uint32_t filterBank, uint32_t FIFO  )
{
    if ( (format == CAN_ID_STD) || (format == CAN_ID_EXT) )
    {
        /** Declare a CAN filter configuration */
        CAN_FilterTypeDef  sFilterConfig;

        /** Verify the filter bank is possible */
        if ( (filterBank >= 0) && (filterBank <= 13) )
            sFilterConfig.FilterBank = filterBank;
        else
            return -1;

        sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
        sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

        if (format == CAN_ID_STD) {
            sFilterConfig.FilterIdHigh = ((id << 5) | (id >> (32 - 5))) & 0xFFFF;
            sFilterConfig.FilterIdLow =  (id >> (11-3)) & 0xFFF8;
            sFilterConfig.FilterMaskIdHigh = ((mask << 5) | (mask >> (32-5))) & 0xFFFF;
            sFilterConfig.FilterMaskIdLow = (mask >> (11-3)) & 0xFFF8;
        } else { // format == CANExtended
            sFilterConfig.FilterIdHigh = id >> 13; // EXTID[28:13]
            sFilterConfig.FilterIdLow = (0xFFFF & (id << 3)) | (1 << 2); // EXTID[12:0] + IDE
            sFilterConfig.FilterMaskIdHigh = mask >> 13;
            sFilterConfig.FilterMaskIdLow = (0xFFFF & (mask << 3)) | (1 << 2);
        }

        sFilterConfig.FilterFIFOAssignment = FIFO;
        sFilterConfig.FilterActivation = ENABLE;
        sFilterConfig.FilterBank = filterBank;
        sFilterConfig.SlaveStartFilterBank = 0x12;

        return HAL_CAN_ConfigFilter(pcan, &sFilterConfig);
    }
    return HAL_ERROR;
}

static uint8_t Engine_RPM_Tx_CAN_Bus()
{
    uint32_t pTxMailbox = 0;

    uint8_t tx_buf[8];

    /* Create your custom payload here */
    tx_buf[0] = 0x03;
    tx_buf[1] = 0x41;
    tx_buf[2] = 0x0C;
    tx_buf[3] = 0x12;
    tx_buf[4] = (uint8_t)(((engine_rpm / 2) >> 8) & 0xFF);
    tx_buf[5] = (uint8_t)((engine_rpm / 2) & 0xFF);
    tx_buf[6] = 0xff;
    tx_buf[7] = 0xff;

    if( HAL_CAN_AddTxMessage( &hcan, &Header, tx_buf, &pTxMailbox ) == HAL_OK )
        return 1;
    else
        return 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_TIM1_Init();
  MX_TIM16_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_IC_Start_IT( RPM_TIM,  TIM_CHANNEL_1 );

  if( can_filter( &hcan, 0x090, 0x7FF, CAN_ID_STD, 0, CAN_FILTER_FIFO0 ) != HAL_OK )
      Error_Handler();

  if( HAL_CAN_Start( &hcan ) != HAL_OK )
      Error_Handler();

  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

      if( HAL_GetTick() > blink )
      {
          blink = HAL_GetTick() + blink_rate;
          HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
      }
      //Engine_RPM_Tx_CAN_Bus();
      //HAL_Delay(100);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

char msg[64];
uint32_t len = 0;

void HAL_TIM_IC_CaptureCallback( TIM_HandleTypeDef *htim )
{
    if ( htim == RPM_TIM )
    {
        t1 = 0;
        htim->Instance->CNT = 0;
        t2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        if( t2 > 0 )
        {
           engine_rpm = 300000 / t2;
           len = snprintf(msg, 64, "%u\n", (uint)engine_rpm);
           //HAL_UART_Transmit_IT(&huart2, (uint8_t*)msg, len);
           Engine_RPM_Tx_CAN_Bus();
        }
        //HAL_GPIO_TogglePin(DEBUG_OUT_GPIO_Port, DEBUG_OUT_Pin);
        //engine_rpm = ((t2 - t1) * ENGINE_CYL_FACTOR * RPS2RPM);
        //engine_rpm = (RPM_TIM_UNITS * 1000) / (( t2 - t1 ) / ENGINE_CYL_FACTOR * RPS2RPM);
    }
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
void assert_failed(char *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
