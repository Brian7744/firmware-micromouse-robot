/**
 * @file motor.h
 * @brief Driver para control de 2 motores N20 via L9110S
 *
 * Motor 1 (izquierdo): PB5=M1A (TIM3_CH2), PB4=M1B (TIM3_CH1) - Full hardware PWM
 * Motor 2 (derecho):   PB3=M2A (TIM2_CH2), PB15=M2B (GPIO)    - PWM + locked-antiphase
 *
 * Rango de velocidad: -MOTOR_PWM_MAX a +MOTOR_PWM_MAX
 * Positivo = adelante, Negativo = reversa
 */
#ifndef MOTOR_H
#define MOTOR_H

#include "main.h"

/* ---- Configuracion ---- */
// PWM a 100Hz: 72MHz / (PSC+1) / (ARR+1) = 72M / 72 / 10000 = 100Hz
// Resolucion: 10000 niveles (0-9999), cada tick = 1us, periodo = 10ms
#define MOTOR_PWM_MAX   9999    // ARR del timer (100Hz con PSC=71)
#define MOTOR_DEADZONE  100     // Zona muerta para evitar zumbido a baja velocidad

typedef enum {
    MOTOR_LEFT  = 0,
    MOTOR_RIGHT = 1
} Motor_ID_t;

/* ---- API ---- */

/**
 * @brief Inicializa PWM de motores (llama despues de MX_TIMx_Init)
 * @param htim_m1 Handle TIM3 (Motor 1)
 * @param htim_m2 Handle TIM2 (Motor 2)
 */
void Motor_Init(TIM_HandleTypeDef *htim_m1, TIM_HandleTypeDef *htim_m2);

/**
 * @brief Establece velocidad de un motor
 * @param motor MOTOR_LEFT o MOTOR_RIGHT
 * @param speed -MOTOR_PWM_MAX a +MOTOR_PWM_MAX (+ = adelante)
 */
void Motor_SetSpeed(Motor_ID_t motor, int16_t speed);

/**
 * @brief Detiene un motor (coast - rueda libre)
 */
void Motor_Stop(Motor_ID_t motor);

/**
 * @brief Frena ambos motores (brake activo)
 */
void Motor_BrakeAll(void);

/**
 * @brief Obtiene velocidad actual configurada
 */
int16_t Motor_GetSpeed(Motor_ID_t motor);

#endif /* MOTOR_H */
