/**
 * @file ultrasonic.h
 * @brief Driver no bloqueante para sensor ultrasonico HC-SR04
 *
 * PB12 = Trigger (GPIO output)
 * PB13 = Echo    (EXTI rising + falling)
 *
 * Usa DWT CYCCNT para medicion precisa de tiempo (72MHz = 13.9ns/tick)
 * Distancia maxima: ~400cm, minima: ~2cm
 */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "main.h"

/* ---- Configuracion ---- */
#define US_MAX_DISTANCE_CM  400     // Distancia maxima medible
#define US_MIN_DISTANCE_CM  2       // Distancia minima
#define US_TIMEOUT_US       25000   // Timeout de medicion (25ms ~ 4.3m)

/* ---- Estado del sensor ---- */
typedef enum {
    US_IDLE = 0,        // Listo para medir
    US_TRIGGER_SENT,    // Trigger enviado, esperando echo
    US_MEASURING,       // Midiendo (echo HIGH)
    US_DONE,            // Medicion completada
    US_TIMEOUT          // Timeout (sin obstáculo o fuera de rango)
} US_State_t;

typedef struct {
    US_State_t state;
    float distance_cm;          // Ultima distancia valida en cm
    uint32_t echo_start_cycles; // DWT CYCCNT al rising edge del echo
    uint32_t echo_end_cycles;   // DWT CYCCNT al falling edge del echo
    uint8_t  new_data;          // Flag: nueva medicion disponible
} US_Data_t;

/* ---- API ---- */

/**
 * @brief Inicializa el sensor ultrasonico (habilita DWT CYCCNT)
 */
void US_Init(void);

/**
 * @brief Envia pulso de trigger (10us)
 *        Llamar periodicamente (ej: cada 60ms para 400cm max)
 */
void US_Trigger(void);

/**
 * @brief Callback para EXTI de PB13 (llamar desde HAL_GPIO_EXTI_Callback)
 *        Detecta rising y falling edge del echo
 */
void US_EXTI_Callback(void);

/**
 * @brief Obtiene la ultima distancia medida en cm
 * @return Distancia en cm, o -1.0 si timeout/invalida
 */
float US_GetDistance(void);

/**
 * @brief Obtiene puntero a datos completos del sensor
 */
const US_Data_t* US_GetData(void);

/**
 * @brief Verifica si hay un obstaculo a menos de la distancia dada
 */
uint8_t US_IsObstacle(float threshold_cm);

#endif /* ULTRASONIC_H */
