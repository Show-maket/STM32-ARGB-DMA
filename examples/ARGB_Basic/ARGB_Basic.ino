/**
 * @file    ARGB_Basic.ino
 * @brief   Минимальный пример использования STM32-ARGB-DMA
 * 
 * Подключение:
 *   PA0 -> DATA ленты WS2812
 *   GND -> GND ленты
 *   5V  -> VCC ленты
 */

// ============================================================================
// Конфигурация - ДО включения библиотеки!
// ============================================================================
#define NUM_PIXELS 8
#define WS2812
#define USE_GAMMA_CORRECTION 0

#include <ARGB.h>
#include <ARGB_Auto.h>

// Пин для данных (любой PWM-совместимый)
#define ARGB_PIN PA0

// ============================================================================
// DMA IRQ Handler - ЕДИНСТВЕННОЕ что нужно написать вручную!
// Имя зависит от пина, выводится в Serial при запуске.
// ============================================================================
extern "C" void DMA1_Stream5_IRQHandler(void) {
    ARGB_DMA_IRQHandler();
}

// ============================================================================
// Setup & Loop
// ============================================================================
uint8_t hue = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println(F("\n=== ARGB Basic Example ===\n"));
    
    // Вся инициализация в одну строку!
    if (!ARGB_Begin(ARGB_PIN)) {
        Serial.println(F("FATAL: ARGB init failed!"));
        while (1) delay(100);
    }
    
    ARGB_SetBrightness(25);  // 10%
    
    // Тестовый кадр
    ARGB_SetRGB(0, 255, 0, 0);    // Red
    ARGB_SetRGB(1, 0, 255, 0);    // Green  
    ARGB_SetRGB(2, 0, 0, 255);    // Blue
    ARGB_SetRGB(3, 255, 255, 255);// White
    ARGB_Show();
    
    Serial.println(F("Ready!"));
}

void loop() {
    // Rainbow effect
    for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        ARGB_SetHSV(i, hue + i * (256 / NUM_PIXELS), 255, 255);
    }
    hue += 2;
    
    ARGB_Show();
    delay(20);
}
