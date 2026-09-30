/**
 * @file sh1106.c
 * @brief Implementacion del driver OLED SH1106 no bloqueante
 *
 * Framebuffer: 128 x 8 paginas = 1024 bytes
 * Cada byte representa 8 pixeles verticales (LSB = top)
 *
 * Transferencia no bloqueante:
 *   OLED_Update() envia pagina 0 por DMA, luego el callback
 *   HAL_I2C_MemTxCpltCallback envia las paginas restantes.
 */
#include "sh1106.h"
#include <string.h>
#include <stdio.h>

/* ---- Fuente 5x7 embebida (ASCII 32-126) ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
    {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
    {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
    {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
    {0x00,0x05,0x03,0x00,0x00}, // 39 '''
    {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
    {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
    {0x14,0x08,0x3E,0x08,0x14}, // 42 '*'
    {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
    {0x00,0x50,0x30,0x00,0x00}, // 44 ','
    {0x08,0x08,0x08,0x08,0x08}, // 45 '-'
    {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
    {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
    {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
    {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
    {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
    {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
    {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
    {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
    {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
    {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
    {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
    {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
    {0x08,0x14,0x22,0x41,0x00}, // 60 '<'
    {0x14,0x14,0x14,0x14,0x14}, // 61 '='
    {0x00,0x41,0x22,0x14,0x08}, // 62 '>'
    {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
    {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 70 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 71 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 77 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 87 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 89 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // 91 '['
    {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
    {0x00,0x41,0x41,0x7F,0x00}, // 93 ']'
    {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
    {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
    {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
    {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 103 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 107 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 113 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 115 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
    {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
    {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
    {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
    {0x10,0x08,0x08,0x10,0x08}, // 126 '~'
};

/* ---- Variables ---- */
static I2C_HandleTypeDef *hi2c_oled;
static uint8_t framebuffer[OLED_WIDTH * OLED_PAGES];  // 1024 bytes
static OLED_State_t oled_state = OLED_READY;
static uint8_t current_page = 0;

// Buffer de transmision DMA: 3 bytes comando + 128 bytes datos
// Los 3 primeros bytes son: page address, col low, col high
// Enviamos comandos bloqueante (rapido, 3 bytes) y datos por DMA
static uint8_t dma_page_buf[OLED_WIDTH];

/* ---- Funciones internas ---- */

/**
 * @brief Envia un comando al SH1106 (bloqueante, muy rapido: 1 byte)
 */
static void oled_write_cmd(uint8_t cmd)
{
    HAL_I2C_Mem_Write(hi2c_oled, OLED_I2C_ADDR << 1, 0x00,
                      I2C_MEMADD_SIZE_8BIT, &cmd, 1, 10);
}

/**
 * @brief Inicia envio de una pagina del framebuffer por DMA
 */
static void oled_send_page(uint8_t page)
{
    // Configurar posicion: pagina y columna inicio
    oled_write_cmd(0xB0 + page);                        // Set page address
    oled_write_cmd(0x00 + (SH1106_COL_OFFSET & 0x0F));  // Set col addr low nibble
    oled_write_cmd(0x10 + (SH1106_COL_OFFSET >> 4));     // Set col addr high nibble

    // Copiar datos de esta pagina al buffer DMA
    memcpy(dma_page_buf, &framebuffer[page * OLED_WIDTH], OLED_WIDTH);

    // Enviar datos por DMA (no bloqueante)
    HAL_I2C_Mem_Write_DMA(hi2c_oled, OLED_I2C_ADDR << 1, 0x40,
                          I2C_MEMADD_SIZE_8BIT, dma_page_buf, OLED_WIDTH);
}

/* ---- API de control ---- */

int8_t OLED_Init(I2C_HandleTypeDef *hi2c)
{
    hi2c_oled = hi2c;

    HAL_Delay(100);  // Esperar estabilizacion del display

    // Secuencia de inicializacion SH1106 (la que funciono originalmente)
    static const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Set display clock divide ratio
        0xA8, 0x3F, // Set multiplex ratio (64-1)
        0xD3, 0x00, // Set display offset = 0
        0x40,       // Set start line = 0
        0x8D, 0x14, // Set charge pump (internal DC-DC)
        0xA1,       // Segment re-map (col 127 = SEG0) -> flip horizontal
        0xC8,       // COM scan direction reversed -> flip vertical
        0xDA, 0x12, // Set COM pins config (alternative, no remap)
        0x81, 0x80, // Set contrast = 128
        0xD9, 0x1F, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect level
        0x33,       // Set VPP to 9.0V
        0xA6,       // Normal display (not inverted)
        0xA4,       // Display from RAM
        0xAF,       // Display ON
    };

    for (uint8_t i = 0; i < sizeof(init_cmds); i++) {
        oled_write_cmd(init_cmds[i]);
    }

    OLED_Clear();

    // Enviar framebuffer inicial (bloqueante, solo en init)
    for (uint8_t p = 0; p < OLED_PAGES; p++) {
        oled_write_cmd(0xB0 + p);
        oled_write_cmd(0x00 + (SH1106_COL_OFFSET & 0x0F));
        oled_write_cmd(0x10 + (SH1106_COL_OFFSET >> 4));
        HAL_I2C_Mem_Write(hi2c_oled, OLED_I2C_ADDR << 1, 0x40,
                          I2C_MEMADD_SIZE_8BIT,
                          &framebuffer[p * OLED_WIDTH], OLED_WIDTH, 100);
    }

    oled_state = OLED_READY;
    return 0;
}

int8_t OLED_Update(void)
{
    if (oled_state == OLED_BUSY)
        return -1;  // Update anterior en curso

    oled_state = OLED_BUSY;
    current_page = 0;
    oled_send_page(0);
    return 0;
}

void OLED_DMA_TxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c != hi2c_oled) return;

    current_page++;
    if (current_page < OLED_PAGES) {
        // Enviar siguiente pagina
        oled_send_page(current_page);
    } else {
        // Todas las paginas enviadas
        oled_state = OLED_READY;
    }
}

uint8_t OLED_IsReady(void)
{
    return (oled_state == OLED_READY) ? 1 : 0;
}

OLED_State_t OLED_GetState(void)
{
    return oled_state;
}

/* ---- API de dibujo ---- */

void OLED_Clear(void)
{
    memset(framebuffer, 0x00, sizeof(framebuffer));
}

void OLED_Fill(void)
{
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    uint16_t idx = (y / 8) * OLED_WIDTH + x;

    if (color)
        framebuffer[idx] |=  (1 << (y & 7));
    else
        framebuffer[idx] &= ~(1 << (y & 7));
}

void OLED_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t color)
{
    if (ch < 32 || ch > 126) ch = '?';
    uint8_t idx = ch - 32;

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1 << row))
                OLED_DrawPixel(x + col, y + row, color);
            else
                OLED_DrawPixel(x + col, y + row, !color);
        }
    }
    // Espacio entre caracteres (1 pixel)
    for (uint8_t row = 0; row < 8; row++)
        OLED_DrawPixel(x + 5, y + row, !color);
}

void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t color)
{
    while (*str) {
        if (x + 6 > OLED_WIDTH) break;  // No sale de pantalla
        OLED_DrawChar(x, y, *str, color);
        x += 6;  // 5 pixeles + 1 espacio
        str++;
    }
}

void OLED_DrawInt(uint8_t x, uint8_t y, int32_t num, uint8_t color)
{
    char buf[12];
    int len = 0;

    if (num < 0) {
        buf[len++] = '-';
        num = -num;
    }

    // Convertir digitos (al reves)
    char tmp[11];
    int tlen = 0;
    if (num == 0) {
        tmp[tlen++] = '0';
    } else {
        while (num > 0) {
            tmp[tlen++] = '0' + (num % 10);
            num /= 10;
        }
    }

    // Invertir orden
    for (int i = tlen - 1; i >= 0; i--)
        buf[len++] = tmp[i];
    buf[len] = '\0';

    OLED_DrawString(x, y, buf, color);
}

void OLED_DrawFloat(uint8_t x, uint8_t y, float num, uint8_t decimals, uint8_t color)
{
    // Implementacion simple sin sprintf float (ahorra flash)
    if (num < 0) {
        OLED_DrawChar(x, y, '-', color);
        x += 6;
        num = -num;
    }

    int32_t int_part = (int32_t)num;
    OLED_DrawInt(x, y, int_part, color);

    // Avanzar x segun digitos de la parte entera
    int32_t temp = int_part;
    uint8_t digits = (int_part == 0) ? 1 : 0;
    while (temp > 0) { digits++; temp /= 10; }
    x += digits * 6;

    if (decimals > 0) {
        OLED_DrawChar(x, y, '.', color);
        x += 6;

        float frac = num - (float)int_part;
        for (uint8_t d = 0; d < decimals; d++) {
            frac *= 10.0f;
            uint8_t digit = (uint8_t)frac;
            OLED_DrawChar(x, y, '0' + digit, color);
            x += 6;
            frac -= digit;
        }
    }
}

void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
    for (uint8_t i = 0; i < w; i++)
        OLED_DrawPixel(x + i, y, color);
}

void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
    for (uint8_t i = 0; i < h; i++)
        OLED_DrawPixel(x, y + i, color);
}

void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    OLED_DrawHLine(x, y, w, color);
    OLED_DrawHLine(x, y + h - 1, w, color);
    OLED_DrawVLine(x, y, h, color);
    OLED_DrawVLine(x + w - 1, y, h, color);
}

void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t j = 0; j < h; j++)
        for (uint8_t i = 0; i < w; i++)
            OLED_DrawPixel(x + i, y + j, color);
}

void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent)
{
    if (percent > 100) percent = 100;
    OLED_DrawRect(x, y, w, h, 1);
    uint8_t fill_w = ((uint16_t)(w - 2) * percent) / 100;
    if (fill_w > 0)
        OLED_FillRect(x + 1, y + 1, fill_w, h - 2, 1);
}
