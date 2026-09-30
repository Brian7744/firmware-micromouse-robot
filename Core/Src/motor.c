/**
 * @file motor.c
 * @brief Implementacion del driver de motores L9110S
 *
 * L9110S Truth Table:
 *   IA=H, IB=L -> Forward
 *   IA=L, IB=H -> Reverse
 *   IA=L, IB=L -> Coast (rueda libre)
 *   IA=H, IB=H -> Brake (freno activo)
 *
 * Motor 1 (Left): TIM3_CH2(PB5)=M1A, TIM3_CH1(PB4)=M1B -> ambos PWM hardware
 * Motor 2 (Right): TIM2_CH2(PB3)=M2A(PWM), PB15=M2B(GPIO) -> locked-antiphase
 */
#include "motor.h"

/* ---- Handles internos ---- */
static TIM_HandleTypeDef *htim_motor1;  // TIM3
static TIM_HandleTypeDef *htim_motor2;  // TIM2
static int16_t current_speed[2] = {0, 0};

/* ---- Funciones internas ---- */
static int16_t clamp(int16_t val, int16_t min, int16_t max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ---- Motor 1: Full hardware PWM (TIM3 CH1 + CH2) ---- */
static void Motor1_Set(int16_t speed)
{
    speed = clamp(speed, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);

    if (speed > MOTOR_DEADZONE) {
        // Adelante: M1A(CH2)=PWM, M1B(CH1)=0
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, (uint16_t)speed);
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, 0);
    }
    else if (speed < -MOTOR_DEADZONE) {
        // Reversa: M1A(CH2)=0, M1B(CH1)=PWM
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, (uint16_t)(-speed));
    }
    else {
        // Zona muerta -> Coast
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, 0);
    }
    current_speed[MOTOR_LEFT] = speed;
}

/* ---- Motor 2: PWM + GPIO locked-antiphase ---- */
/*
 * Forward: M2B(PB15)=LOW, M2A(PB3)=PWM(duty)
 *   PWM H -> Forward, PWM L -> Coast => neto: forward proporcional a duty
 *
 * Reverse: M2B(PB15)=HIGH, M2A(PB3)=PWM(MAX - |speed|)
 *   PWM H -> Brake, PWM L -> Reverse => neto: reversa proporcional a (MAX - duty)
 *   Con duty = MAX - |speed|: cuando |speed| es maximo, duty=0 -> full reverse
 */
static void Motor2_Set(int16_t speed)
{
    speed = clamp(speed, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);

    if (speed > MOTOR_DEADZONE) {
        // Adelante
        //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);  // M2B = LOW
        __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2, (uint16_t)speed);
    }
    else if (speed < -MOTOR_DEADZONE) {
        // Reversa (locked-antiphase)
        //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);    // M2B = HIGH
        __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2,
                              (uint16_t)(MOTOR_PWM_MAX + speed)); // MAX - |speed|
    }
    else {
        // Zona muerta -> Coast
        //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2, 0);
    }
    current_speed[MOTOR_RIGHT] = speed;
}

/* ---- API Publica ---- */

void Motor_Init(TIM_HandleTypeDef *htim_m1, TIM_HandleTypeDef *htim_m2)
{
    htim_motor1 = htim_m1;
    htim_motor2 = htim_m2;

    // PASO 1: Forzar duty=0 ANTES de arrancar el PWM (evita glitches)
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2, 0);

    // PASO 2: Asegurar GPIO PB15 (M2B) en LOW antes que nada
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
    // PASO 3: Arrancar PWM con duty 0 ya pre-cargado
    HAL_TIM_PWM_Start(htim_motor1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(htim_motor1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(htim_motor2, TIM_CHANNEL_2);

    // PASO 4: Doble seguridad - re-forzar duty=0 despues de start
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2, 0);

    current_speed[0] = 0;
    current_speed[1] = 0;
}

void Motor_SetSpeed(Motor_ID_t motor, int16_t speed)
{
    if (motor == MOTOR_LEFT)
        Motor1_Set(speed);
    else
        Motor2_Set(speed);
}

void Motor_Stop(Motor_ID_t motor)
{
    Motor_SetSpeed(motor, 0);
}

void Motor_BrakeAll(void)
{
    // Motor 1: ambos HIGH = brake
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_1, MOTOR_PWM_MAX);
    __HAL_TIM_SET_COMPARE(htim_motor1, TIM_CHANNEL_2, MOTOR_PWM_MAX);
    current_speed[MOTOR_LEFT] = 0;

    // Motor 2: M2B=HIGH, M2A=HIGH = brake
    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(htim_motor2, TIM_CHANNEL_2, MOTOR_PWM_MAX);
    current_speed[MOTOR_RIGHT] = 0;
}

int16_t Motor_GetSpeed(Motor_ID_t motor)
{
    return current_speed[motor];
}
