/**
 * @file sh1106.h
 * @brief Driver NO BLOQUEANTE para display OLED 128x64 SH1106 via I2C DMA
 *
 * PB8 = SCL (I2C1 remap), PB9 = SDA (I2C1 remap)
 * I2C address: 0x3C (7-bit)
 *
 * Arquitectura no bloqueante:
 *   1. Dibujar en framebuffer (1024 bytes en RAM)
 *   2. Llamar OLED_Update() -> envia pagina 0 por DMA
 *   3. En callback I2C DMA complete -> envia pagina siguiente
 *   4. Despues de 8 paginas, flag "ready" se activa
 *
 * El CPU queda libre mientras DMA transfiere los datos al display.
 */
#ifndef SH1106_H
#define SH1106_H

#include "main.h"

/* ---- Configuracion ---- */
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)   // 8 paginas
#define OLED_I2C_ADDR       0x3C                // 7-bit address
// SH1106 tiene 132 columnas internas pero solo 128 visibles, offset = 2
#define SH1106_COL_OFFSET   0                   // 2 para SH1106

/* ---- Estados del display ---- */
typedef enum {
    OLED_READY = 0,     // Listo para nuevo update
    OLED_BUSY,          // Enviando datos por DMA
    OLED_ERROR          // Error de I2C
} OLED_State_t;

/* ---- API de control ---- */

/**
 * @brief Inicializa el display OLED (secuencia de init commands)
 * @param hi2c Handle del I2C1
 * @return 0=OK, -1=error
 * @note BLOQUEANTE solo durante init (una sola vez)
 */
int8_t OLED_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Envia el framebuffer al display (NO BLOQUEANTE via DMA)
 *        Inicia la transferencia de pagina 0. Las siguientes paginas
 *        se envian desde el callback I2C DMA.
 * @return 0=OK, -1=busy (update anterior en curso)
 */
int8_t OLED_Update(void);

/**
 * @brief Callback para I2C DMA TX complete
 *        DEBE llamarse desde HAL_I2C_MemTxCpltCallback()
 */
void OLED_DMA_TxCpltCallback(I2C_HandleTypeDef *hi2c);

/**
 * @brief Verifica si el display esta listo para un nuevo update
 */
uint8_t OLED_IsReady(void);

/**
 * @brief Obtiene estado actual del display
 */
OLED_State_t OLED_GetState(void);

/* ---- API de dibujo (operan sobre el framebuffer, no bloquean) ---- */

/**
 * @brief Limpia el framebuffer (todo negro)
 */
void OLED_Clear(void);

/**
 * @brief Llena todo el framebuffer (todo blanco)
 */
void OLED_Fill(void);

/**
 * @brief Dibuja un pixel
 * @param x Columna (0 a 127)
 * @param y Fila (0 a 63)
 * @param color 1=blanco, 0=negro
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Escribe un caracter con fuente 5x7
 * @param x Columna de inicio
 * @param y Fila de inicio
 * @param ch Caracter ASCII (32-126)
 * @param color 1=blanco sobre negro, 0=negro sobre blanco
 */
void OLED_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t color);

/**
 * @brief Escribe un string
 */
void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t color);

/**
 * @brief Escribe un numero entero
 */
void OLED_DrawInt(uint8_t x, uint8_t y, int32_t num, uint8_t color);

/**
 * @brief Escribe un float con decimales
 */
void OLED_DrawFloat(uint8_t x, uint8_t y, float num, uint8_t decimals, uint8_t color);

/**
 * @brief Dibuja una linea horizontal
 */
void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t w, uint8_t color);

/**
 * @brief Dibuja una linea vertical
 */
void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t h, uint8_t color);

/**
 * @brief Dibuja un rectangulo (solo borde)
 */
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/**
 * @brief Dibuja un rectangulo relleno
 */
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/**
 * @brief Dibuja una barra de progreso
 * @param x,y Posicion
 * @param w,h Tamano
 * @param percent Porcentaje (0-100)
 */
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent);

#endif /* SH1106_H */
