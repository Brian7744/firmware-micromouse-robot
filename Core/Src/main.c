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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>
#include "sh1106.h"
#include "motor.h"
#include "ultrasonic.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum{
    BUTTON_UP,
    BUTTON_FALLING,
    BUTTON_DOWN,
    BUTTON_RISING
} _eButtonState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* handler declaration*/
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

uint8_t button_pressed_flag = 0;
_eButtonState button_state = BUTTON_UP;

/*Variable para debuggear el ultrasonico*/
volatile uint32_t eco_interrupciones = 0;

char msg[32];
uint8_t texto[] = "Mensaje STM32\r\n";
uint8_t flagSec = 0;

/*Variable de prueba*/
uint32_t valores = 10;

/*Variables para el USB*/
uint8_t  BufUSBRx[256];
uint8_t  nByteTx = 0;
uint8_t  flagUSBRx = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim1);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);
void button_update();
void trans_por_uart1();
void Test_BothMotors(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void Test_Ultrasonic(void);
void USBRXX(uint8_t *Buf, uint32_t Len);
void Servo_SetAngle(uint8_t angle);
void Test_Servo(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // Necesario para el control de los motores
  __HAL_RCC_AFIO_CLK_ENABLE();      // Habilita reloj de funciones alternativas
  __HAL_AFIO_REMAP_SWJ_NOJTAG();    // Libera PB3, PB4 y PB15 apagando el JTAG

  // === INICIO TIM1 (Botón + Servo) ===
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // Enciende la señal PWM en PA8

  Motor_Init(&htim3, &htim2);
  // === INICIO OLED ===
  //if (OLED_Init(&hi2c1) == 0) {
  //    OLED_Clear();
  //    OLED_DrawString(10, 10, "DISPLAY OK!", 1);
  //    OLED_DrawString(10, 30, "Modo: FASE 2", 1);
  //    OLED_Update(); // Dispara el envío por DMA
  //}
  OLED_Init(&hi2c1);
  //Test_BothMotors();
  //HAL_USART_Init(&huart1);
  Test_Ultrasonic();
  //Test_Servo();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    if(flagUSBRx){
	        if(CDC_Transmit_FS(BufUSBRx, nByteTx) == USBD_OK){
	            flagUSBRx = 0;   // limpia solo si se envió bien
	        }
	    }

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void button_update(){

    switch(button_state)
    {
        case BUTTON_UP:
            if (!HAL_GPIO_ReadPin(SW0_GPIO_Port, SW0_Pin)) {
                button_state = BUTTON_FALLING;
            }
            break;

        case BUTTON_FALLING:
            if (!HAL_GPIO_ReadPin(SW0_GPIO_Port, SW0_Pin)) {
                button_state = BUTTON_DOWN;
                button_pressed_flag++;
            } else {
                button_state = BUTTON_UP;
            }
            break;

        case BUTTON_DOWN:
            if (HAL_GPIO_ReadPin(SW0_GPIO_Port, SW0_Pin)) {
                button_state = BUTTON_RISING;
            }
            break;

        case BUTTON_RISING:
            if (HAL_GPIO_ReadPin(SW0_GPIO_Port, SW0_Pin)) {
                button_state = BUTTON_UP;
                //button_pressed_flag=0;
            } else {
                button_state = BUTTON_DOWN;
            }
            break;

        default:
            button_state = BUTTON_UP;
            break;
    }

    if(button_pressed_flag==2){
    	button_pressed_flag=0;
    }
}

void trans_por_uart1(){

	flagSec++;
	if(flagSec>=9){

		sprintf(msg,"enviado por usart1. %lu\r\n", valores);
		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

		flagSec=0;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim1){

	// Esta variable no pierde su valor entre interrupciones
	static uint8_t divisor_100ms = 0;

	if(htim1->Instance == TIM1){
		// 1. El botón se actualiza cada 20ms
		button_update();

		// 2. Escalador: Solo entramos acá 1 de cada 5 veces
		divisor_100ms++;
		if(divisor_100ms >= 5){
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			if(button_pressed_flag){
				HAL_GPIO_TogglePin(LEDEX_GPIO_Port, LEDEX_Pin);
			    trans_por_uart1(); // Sigue transmitiendo al mismo ritmo de antes
			} else {
			    HAL_GPIO_WritePin(LEDEX_GPIO_Port, LEDEX_Pin, GPIO_PIN_RESET);
			}
			divisor_100ms = 0; // Reiniciamos el contador
		}

	}

}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c){
    // Esta función la llama el hardware automáticamente cuando el DMA termina
    OLED_DMA_TxCpltCallback(hi2c);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    // Si la interrupción viene del pin 13 (Echo del sensor)
    if(GPIO_Pin == GPIO_PIN_13){
    	eco_interrupciones++;
    	US_EXTI_Callback();
    }
}


void Test_Ultrasonic(void){
    // Inicializamos el sensor (activa el DWT internamente)
    US_Init();

    // Pantalla de bienvenida
    OLED_Clear();
    OLED_DrawString(10, 0, "SENSOR ACTIVO", 1);
    OLED_Update();
    HAL_Delay(1000);

    while (1) {

        // 1. Disparamos la ráfaga de ultrasonido
        US_Trigger();

        // 2. Esperamos un tiempo prudencial para que el sonido vuelva (max 25ms)
        HAL_Delay(60);

        // 3. Le pedimos a la librería que haga el cálculo
        float dist = US_GetDistance();

        // --- ACTUALIZACIÓN DE PANTALLA ---
        OLED_Clear();
        OLED_DrawString(10, 0, "TEST ULTRASONICO", 1);
        OLED_DrawHLine(0, 10, 128, 1);

        if (dist > 0) {
            OLED_DrawString(0, 30, "Distancia:", 1);
            // Dibujamos el float directo con 1 decimal
            OLED_DrawFloat(65, 30, dist, 1, 1);
            OLED_DrawString(105, 30, "cm", 1);

            // Barra gráfica para visualizar el rebote
            uint8_t barra = (uint8_t)(dist);
            if(barra > 128) barra = 128;
            OLED_FillRect(0, 50, barra, 10, 1);
        } else {
            // Si el driver devuelve -1.0f (timeout o fuera de rango)
            OLED_DrawString(0, 30, "Sin lectura", 1);
        }

        // Mandamos el frame por DMA
        OLED_Update();

        // Completamos el ciclo para no saturar la pantalla
        HAL_Delay(440);
    }
}


/**
  * @brief  Mueve el servo a un ángulo específico (0 a 180 grados)
  * @param  angle: Grados deseados
  */
void Servo_SetAngle(uint8_t angle){
    if(angle > 180) angle = 180;

    // Mapeo lineal: 0° -> 0.5ms (500 cuentas) | 180° -> 2.5ms (2500 cuentas)
    uint32_t compare_value = 500 + ((angle * 2000) / 180);

    // Cargamos el registro del TIM1 Canal 1 (Pin PA8)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_value);
}

void Test_Servo(void){
    while(1)
    {
        Servo_SetAngle(0);
        HAL_Delay(1000);

        Servo_SetAngle(90);
        HAL_Delay(1000);

        Servo_SetAngle(180);
        HAL_Delay(1000);

        Servo_SetAngle(90);
        HAL_Delay(1000);
    }
}

void Test_BothMotors(void){
    // Inicializa los drivers usando los timers de tu main
    Motor_Init(&htim3, &htim2);

    int16_t speed = 5000;  // 50% de la velocidad máxima (9999)

    while (1) {
        // Adelante
        Motor_SetSpeed(MOTOR_LEFT, speed);
        Motor_SetSpeed(MOTOR_RIGHT, speed);
        HAL_Delay(2000);

        // Frenar
        //Motor_BrakeAll();
        //HAL_Delay(1000);

        // Reversa
        //Motor_SetSpeed(MOTOR_LEFT, -speed);
        //Motor_SetSpeed(MOTOR_RIGHT, -speed);
        //HAL_Delay(2000);

        // Frenar
        //Motor_BrakeAll();
        //HAL_Delay(1000);

        // Giro derecha (izq adelante, der reversa)
        //Motor_SetSpeed(MOTOR_LEFT, speed);
        //Motor_SetSpeed(MOTOR_RIGHT, -speed);
        //HAL_Delay(1500);

        //Motor_BrakeAll();
        //HAL_Delay(1000);

        // Giro izquierda
        //Motor_SetSpeed(MOTOR_LEFT, -speed);
        //Motor_SetSpeed(MOTOR_RIGHT, speed);
        //HAL_Delay(1500);

        //Motor_BrakeAll();
        //HAL_Delay(1000);
    }
}

void USBRXX(uint8_t *Buf, uint32_t Len){

    BufUSBRx[0] = 'U';
    BufUSBRx[1] = 'S';
    BufUSBRx[2] = 'B';
    BufUSBRx[3] = ' ';

    for(uint32_t i=0; i<Len; i++){
        BufUSBRx[i+4] = Buf[i];
    }

    nByteTx   = Len + 4;
    flagUSBRx = 1;          // avisa al while(1) que hay dato
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
