/**
 * @file ultrasonic.c
 * @brief Implementacion del driver HC-SR04
 *
 * Funcionamiento:
 * 1. Enviar pulso HIGH de 10us en Trigger (PB12)
 * 2. Esperar rising edge en Echo (PB13) -> marca inicio
 * 3. Esperar falling edge en Echo -> marca fin
 * 4. Distancia = (tiempo_echo_us / 2) / 29.1 cm
 *    O equivalente: distancia_cm = tiempo_echo_us / 58
 *
 * DWT CYCCNT: contador de 32 bits a frecuencia del CPU (72MHz)
 *   1 ciclo = 13.89 ns
 *   Para convertir a us: cycles / 72
 */
#include "ultrasonic.h"

/* ---- Variables ---- */
static US_Data_t us_data;

/* ---- DWT (Data Watchpoint and Trace) ---- */
static void DWT_Init(void)
{
    // Habilitar acceso al DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

/* ---- Delay preciso en microsegundos usando DWT ---- */
static void delay_us(uint32_t us)
{
    uint32_t start = DWT_GetCycles();
    uint32_t target = us * (SystemCoreClock / 1000000);
    while ((DWT_GetCycles() - start) < target);
}

/* ---- API ---- */

void US_Init(void)
{
    DWT_Init();

    us_data.state = US_IDLE;
    us_data.distance_cm = -1.0f;
    us_data.echo_start_cycles = 0;
    us_data.echo_end_cycles = 0;
    us_data.new_data = 0;

    // Asegurar trigger LOW
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

void US_Trigger(void)
{
    if (us_data.state != US_IDLE && us_data.state != US_DONE
        && us_data.state != US_TIMEOUT)
        return;  // Medicion en curso

    us_data.state = US_TRIGGER_SENT;
    us_data.new_data = 0;

    // Pulso de trigger: 10us HIGH
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

void US_EXTI_Callback(void)
{
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_SET) {
        // Rising edge: inicio del echo
        us_data.echo_start_cycles = DWT_GetCycles();
        us_data.state = US_MEASURING;
    }
    else {
        // Falling edge: fin del echo
        if (us_data.state == US_MEASURING) {
            us_data.echo_end_cycles = DWT_GetCycles();

            // Calcular duracion en microsegundos
            uint32_t elapsed_cycles = us_data.echo_end_cycles - us_data.echo_start_cycles;
            uint32_t elapsed_us = elapsed_cycles / (SystemCoreClock / 1000000);

            if (elapsed_us < US_TIMEOUT_US) {
                // Distancia = tiempo_ida_y_vuelta / 58 (en cm)
                us_data.distance_cm = (float)elapsed_us / 58.0f;

                // Limitar al rango valido
                if (us_data.distance_cm < US_MIN_DISTANCE_CM)
                    us_data.distance_cm = US_MIN_DISTANCE_CM;
                if (us_data.distance_cm > US_MAX_DISTANCE_CM)
                    us_data.distance_cm = US_MAX_DISTANCE_CM;

                us_data.state = US_DONE;
                us_data.new_data = 1;
            }
            else {
                us_data.distance_cm = -1.0f;
                us_data.state = US_TIMEOUT;
            }
        }
    }
}

float US_GetDistance(void)
{
    return us_data.distance_cm;
}

const US_Data_t* US_GetData(void)
{
    return &us_data;
}

uint8_t US_IsObstacle(float threshold_cm)
{
    if (us_data.distance_cm < 0) return 0;  // Sin datos validos
    return (us_data.distance_cm <= threshold_cm) ? 1 : 0;
}
