/**
 * @file    ARGB_Auto.h
 * @brief   Автоматический анализ пинов для ARGB библиотеки
 * @details Определяет таймер, канал, DMA стрим и другие параметры по номеру пина
 * 
 * @author  DashyFox (Arduino/STM32duino port)
 * @note    Поддерживаемые семейства: STM32F4xx
 *          Для добавления новых - см. CONTRIBUTING.md
 * 
 * @note    This file is part of Arduino/STM32duino port of STM32-ARGB-DMA library.
 *          Original library: https://github.com/Crazy-Geeks/STM32-ARGB-DMA
 */

#ifndef ARGB_AUTO_H
#define ARGB_AUTO_H

#include "stm32_def.h"
#include "PinAF_STM32F1.h"  // Arduino STM32 core pin definitions
#include "PeripheralPins.h"  // pinmap_peripheral, etc.

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Результат анализа пина
// ============================================================================
typedef enum {
    ARGB_DMA_OK = 0,           ///< Успешно
    ARGB_DMA_NO_PWM,           ///< Пин не поддерживает PWM
    ARGB_DMA_NO_DMA,           ///< Таймер не имеет DMA для этого канала
    ARGB_DMA_UNSUPPORTED_TIM,  ///< Таймер не поддерживается
    ARGB_DMA_ERR               ///< Общая ошибка
} ARGB_DMA_Result_t;

// ============================================================================
// Конфигурация пина
// ============================================================================
typedef struct {
    // GPIO
    GPIO_TypeDef*       gpio_port;      ///< Порт GPIO (GPIOA, GPIOB, ...)
    uint32_t            gpio_pin;       ///< Номер пина (GPIO_PIN_0, ...)
    
    // Timer
    TIM_TypeDef*        tim;            ///< Таймер (TIM1, TIM2, ...)
    uint32_t            tim_channel;    ///< Канал (TIM_CHANNEL_1, ...)
    uint32_t            tim_af;         ///< Alternate function (GPIO_AF1_TIM2, ...)
    uint32_t            tim_dma_cc;     ///< DMA CC ID (TIM_DMA_CC1, ...)
    uint8_t             is_32bit_tim;   ///< 1 если таймер 32-битный
    
    // DMA
    DMA_Stream_TypeDef* dma_stream;     ///< DMA стрим
    uint32_t            dma_channel;    ///< DMA канал
    IRQn_Type           dma_irqn;       ///< Номер прерывания
} ARGB_PinConfig_t;

// ============================================================================
// Таблица DMA маппинга для STM32F4xx
// Формат: {TIM, CH, DMA, Stream, Channel, IRQn}
// ============================================================================
#if defined(STM32F4xx)
typedef struct {
    TIM_TypeDef*        tim;
    uint32_t            channel;        // TIM_CHANNEL_x
    DMA_TypeDef*        dma;
    DMA_Stream_TypeDef* stream;
    uint32_t            dma_ch;         // DMA_CHANNEL_x
    IRQn_Type           irqn;
} ARGB_DMA_Map_t;

// STM32F4 DMA Request Mapping (Table 28 from Reference Manual)
static const ARGB_DMA_Map_t ARGB_DMA_MAP[] = {
    // TIM1
    #if defined(TIM1)
    {TIM1, TIM_CHANNEL_1, DMA2, DMA2_Stream1, DMA_CHANNEL_6, DMA2_Stream1_IRQn},
    {TIM1, TIM_CHANNEL_1, DMA2, DMA2_Stream3, DMA_CHANNEL_6, DMA2_Stream3_IRQn},
    {TIM1, TIM_CHANNEL_2, DMA2, DMA2_Stream2, DMA_CHANNEL_6, DMA2_Stream2_IRQn},
    {TIM1, TIM_CHANNEL_3, DMA2, DMA2_Stream6, DMA_CHANNEL_6, DMA2_Stream6_IRQn},
    {TIM1, TIM_CHANNEL_4, DMA2, DMA2_Stream4, DMA_CHANNEL_6, DMA2_Stream4_IRQn},
    #endif
    
    // TIM2
    #if defined(TIM2)
    {TIM2, TIM_CHANNEL_1, DMA1, DMA1_Stream5, DMA_CHANNEL_3, DMA1_Stream5_IRQn},
    {TIM2, TIM_CHANNEL_2, DMA1, DMA1_Stream6, DMA_CHANNEL_3, DMA1_Stream6_IRQn},
    {TIM2, TIM_CHANNEL_3, DMA1, DMA1_Stream1, DMA_CHANNEL_3, DMA1_Stream1_IRQn},
    {TIM2, TIM_CHANNEL_4, DMA1, DMA1_Stream6, DMA_CHANNEL_3, DMA1_Stream6_IRQn},
    {TIM2, TIM_CHANNEL_4, DMA1, DMA1_Stream7, DMA_CHANNEL_3, DMA1_Stream7_IRQn},
    #endif
    
    // TIM3
    #if defined(TIM3)
    {TIM3, TIM_CHANNEL_1, DMA1, DMA1_Stream4, DMA_CHANNEL_5, DMA1_Stream4_IRQn},
    {TIM3, TIM_CHANNEL_2, DMA1, DMA1_Stream5, DMA_CHANNEL_5, DMA1_Stream5_IRQn},
    {TIM3, TIM_CHANNEL_3, DMA1, DMA1_Stream7, DMA_CHANNEL_5, DMA1_Stream7_IRQn},
    {TIM3, TIM_CHANNEL_4, DMA1, DMA1_Stream2, DMA_CHANNEL_5, DMA1_Stream2_IRQn},
    #endif
    
    // TIM4
    #if defined(TIM4)
    {TIM4, TIM_CHANNEL_1, DMA1, DMA1_Stream0, DMA_CHANNEL_2, DMA1_Stream0_IRQn},
    {TIM4, TIM_CHANNEL_2, DMA1, DMA1_Stream3, DMA_CHANNEL_2, DMA1_Stream3_IRQn},
    {TIM4, TIM_CHANNEL_3, DMA1, DMA1_Stream7, DMA_CHANNEL_2, DMA1_Stream7_IRQn},
    #endif
    
    // TIM5
    #if defined(TIM5)
    {TIM5, TIM_CHANNEL_1, DMA1, DMA1_Stream2, DMA_CHANNEL_6, DMA1_Stream2_IRQn},
    {TIM5, TIM_CHANNEL_2, DMA1, DMA1_Stream4, DMA_CHANNEL_6, DMA1_Stream4_IRQn},
    {TIM5, TIM_CHANNEL_3, DMA1, DMA1_Stream0, DMA_CHANNEL_6, DMA1_Stream0_IRQn},
    {TIM5, TIM_CHANNEL_4, DMA1, DMA1_Stream1, DMA_CHANNEL_6, DMA1_Stream1_IRQn},
    {TIM5, TIM_CHANNEL_4, DMA1, DMA1_Stream3, DMA_CHANNEL_6, DMA1_Stream3_IRQn},
    #endif
    
    // Sentinel
    {NULL, 0, NULL, NULL, 0, (IRQn_Type)0}
};
#endif // STM32F4xx

// ============================================================================
// Функции
// ============================================================================

/**
 * @brief  Анализирует пин и заполняет конфигурацию
 * @param  arduino_pin  Arduino-номер пина (PA0, PB1, etc.)
 * @param  cfg          Указатель на структуру конфигурации
 * @return Результат анализа
 */
static inline ARGB_DMA_Result_t ARGB_AnalyzePin(uint32_t arduino_pin, ARGB_PinConfig_t* cfg)
{
    if (cfg == NULL) return ARGB_DMA_ERR;
    
    // Очищаем структуру
    memset(cfg, 0, sizeof(ARGB_PinConfig_t));
    
    // Получаем порт и пин
    PinName pn = digitalPinToPinName(arduino_pin);
    if (pn == NC) return ARGB_DMA_ERR;
    
    cfg->gpio_port = get_GPIO_Port(STM_PORT(pn));
    cfg->gpio_pin = STM_GPIO_PIN(pn);
    
    if (cfg->gpio_port == NULL) return ARGB_DMA_ERR;
    
    // Ищем таймер с PWM на этом пине
    // Используем pinmap из Arduino STM32 core
    const PinMap* map = PinMap_PWM;
    while (map->pin != NC) {
        if (map->pin == pn) {
            // Нашли PWM-совместимый пин
            cfg->tim = (TIM_TypeDef*)map->peripheral;
            cfg->tim_af = STM_PIN_AFNUM(map->function);
            
            // Определяем канал из function field
            uint32_t channel_n = STM_PIN_CHANNEL(map->function);
            switch (channel_n) {
                case 1: cfg->tim_channel = TIM_CHANNEL_1; cfg->tim_dma_cc = TIM_DMA_CC1; break;
                case 2: cfg->tim_channel = TIM_CHANNEL_2; cfg->tim_dma_cc = TIM_DMA_CC2; break;
                case 3: cfg->tim_channel = TIM_CHANNEL_3; cfg->tim_dma_cc = TIM_DMA_CC3; break;
                case 4: cfg->tim_channel = TIM_CHANNEL_4; cfg->tim_dma_cc = TIM_DMA_CC4; break;
                default: continue;  // Неизвестный канал, пробуем следующий
            }
            break;
        }
        map++;
    }
    
    if (cfg->tim == NULL) return ARGB_DMA_NO_PWM;
    
    // Определяем разрядность таймера
    #if defined(TIM2)
    if (cfg->tim == TIM2) cfg->is_32bit_tim = 1;
    #endif
    #if defined(TIM5)
    if (cfg->tim == TIM5) cfg->is_32bit_tim = 1;
    #endif
    
    // Ищем DMA маппинг для этого таймера и канала
    #if defined(STM32F4xx)
    const ARGB_DMA_Map_t* dma_map = ARGB_DMA_MAP;
    while (dma_map->tim != NULL) {
        if (dma_map->tim == cfg->tim && dma_map->channel == cfg->tim_channel) {
            cfg->dma_stream = dma_map->stream;
            cfg->dma_channel = dma_map->dma_ch;
            cfg->dma_irqn = dma_map->irqn;
            return ARGB_DMA_OK;
        }
        dma_map++;
    }
    #else
    #warning "ARGB_Auto: DMA mapping not implemented for this MCU family"
    #endif
    
    return ARGB_DMA_NO_DMA;
}

/**
 * @brief  Возвращает строку с описанием результата
 */
static inline const char* ARGB_DMA_ResultString(ARGB_DMA_Result_t res)
{
    switch (res) {
        case ARGB_DMA_OK:              return "OK";
        case ARGB_DMA_NO_PWM:          return "NO_PWM (pin doesn't support PWM)";
        case ARGB_DMA_NO_DMA:          return "NO_DMA (timer/channel has no DMA)";
        case ARGB_DMA_UNSUPPORTED_TIM: return "UNSUPPORTED_TIM";
        default:                       return "ERROR";
    }
}

// ============================================================================
// Глобальные handles (используются ARGB_Setup и IRQ Handler)
// ============================================================================
static TIM_HandleTypeDef  ARGB_htim;
static DMA_HandleTypeDef  ARGB_hdma;
static ARGB_PinConfig_t   ARGB_cfg;

/**
 * @brief  Универсальная инициализация ARGB по номеру пина
 * @param  arduino_pin  Любой PWM-совместимый пин (PA0, PB4, etc.)
 * @return ARGB_DMA_OK при успехе
 * 
 * Функция автоматически:
 *   1. Определяет какой TIM/Channel/DMA нужен для этого пина
 *   2. Настраивает GPIO, Timer, DMA
 *   3. Вызывает ARGB_Attach() и ARGB_Init()
 */
static inline ARGB_DMA_Result_t ARGB_Setup(uint32_t arduino_pin)
{
    ARGB_DMA_Result_t res = ARGB_AnalyzePin(arduino_pin, &ARGB_cfg);
    if (res != ARGB_DMA_OK) return res;
    
    // GPIO clock
    #ifdef GPIOA
    if (ARGB_cfg.gpio_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    #endif
    #ifdef GPIOB
    if (ARGB_cfg.gpio_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    #endif
    #ifdef GPIOC
    if (ARGB_cfg.gpio_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    #endif
    
    // Timer clock
    #ifdef TIM1
    if (ARGB_cfg.tim == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    #endif
    #ifdef TIM2
    if (ARGB_cfg.tim == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    #endif
    #ifdef TIM3
    if (ARGB_cfg.tim == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    #endif
    #ifdef TIM4
    if (ARGB_cfg.tim == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    #endif
    #ifdef TIM5
    if (ARGB_cfg.tim == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
    #endif
    
    // DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    #ifdef DMA2
    __HAL_RCC_DMA2_CLK_ENABLE();
    #endif
    
    // GPIO
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = ARGB_cfg.gpio_pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = ARGB_cfg.tim_af;
    HAL_GPIO_Init(ARGB_cfg.gpio_port, &gpio);
    
    // Timer
    ARGB_htim.Instance = ARGB_cfg.tim;
    ARGB_htim.Init.Prescaler = 0;
    ARGB_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    ARGB_htim.Init.Period = 104;
    ARGB_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&ARGB_htim);
    
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&ARGB_htim, &oc, ARGB_cfg.tim_channel);
    
    // DMA (автоматически WORD для 32-bit таймеров, HALFWORD для 16-bit)
    ARGB_hdma.Instance = ARGB_cfg.dma_stream;
    ARGB_hdma.Init.Channel = ARGB_cfg.dma_channel;
    ARGB_hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    ARGB_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    ARGB_hdma.Init.MemInc = DMA_MINC_ENABLE;
    ARGB_hdma.Init.PeriphDataAlignment = ARGB_cfg.is_32bit_tim ? DMA_PDATAALIGN_WORD : DMA_PDATAALIGN_HALFWORD;
    ARGB_hdma.Init.MemDataAlignment = ARGB_cfg.is_32bit_tim ? DMA_MDATAALIGN_WORD : DMA_MDATAALIGN_HALFWORD;
    ARGB_hdma.Init.Mode = DMA_NORMAL;
    ARGB_hdma.Init.Priority = DMA_PRIORITY_HIGH;
    ARGB_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&ARGB_hdma);
    
    // Link DMA to Timer
    uint32_t dma_id = (ARGB_cfg.tim_channel == TIM_CHANNEL_1) ? TIM_DMA_ID_CC1 :
                      (ARGB_cfg.tim_channel == TIM_CHANNEL_2) ? TIM_DMA_ID_CC2 :
                      (ARGB_cfg.tim_channel == TIM_CHANNEL_3) ? TIM_DMA_ID_CC3 : TIM_DMA_ID_CC4;
    __HAL_LINKDMA(&ARGB_htim, hdma[dma_id], ARGB_hdma);
    
    // Enable DMA IRQ
    HAL_NVIC_SetPriority(ARGB_cfg.dma_irqn, 1, 0);
    HAL_NVIC_EnableIRQ(ARGB_cfg.dma_irqn);
    
    // Calculate timer clock
    uint32_t tim_clk = 0;
    #if defined(TIM2) || defined(TIM3) || defined(TIM4) || defined(TIM5)
    if (ARGB_cfg.tim == TIM2 || ARGB_cfg.tim == TIM3 || ARGB_cfg.tim == TIM4 || ARGB_cfg.tim == TIM5) {
        tim_clk = HAL_RCC_GetPCLK1Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) tim_clk *= 2;
    }
    #endif
    #ifdef TIM1
    if (ARGB_cfg.tim == TIM1) {
        tim_clk = HAL_RCC_GetPCLK2Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0) tim_clk *= 2;
    }
    #endif
    
    // Pass to ARGB library
    ARGB_Attach(&ARGB_htim, ARGB_cfg.tim_channel, &ARGB_hdma, tim_clk);
    ARGB_Init();
    
    return ARGB_DMA_OK;
}

/** 
 * @brief  Вызывайте из DMA IRQ Handler в скетче
 * @note   Имя IRQ Handler зависит от пина (выводится в Serial)
 */
static inline void ARGB_DMA_IRQHandler(void) {
    HAL_DMA_IRQHandler(&ARGB_hdma);
}

/** @brief  Возвращает IRQn для текущей конфигурации */
static inline IRQn_Type ARGB_GetIRQn(void) {
    return ARGB_cfg.dma_irqn;
}

#ifdef __cplusplus
} // extern "C"

// ============================================================================
// C++ functions (require Arduino Serial)
// ============================================================================

// Forward declaration
static inline void ARGB_PrintPinConfig(uint32_t arduino_pin);

/**
 * @brief  Простая инициализация — вызвать и забыть
 * @param  arduino_pin  Любой PWM пин (PA0, PB4, etc.)
 * @return true при успехе
 */
static inline bool ARGB_Begin(uint32_t arduino_pin) {
    ARGB_DMA_Result_t res = ARGB_Setup(arduino_pin);
    if (res != ARGB_DMA_OK) {
        Serial.print(F("ARGB Error: "));
        Serial.println(ARGB_DMA_ResultString(res));
        return false;
    }
    Serial.print(F("ARGB: "));
    ARGB_PrintPinConfig(arduino_pin);
    return true;
}

/**
 * @brief  Выводит конфигурацию в Serial
 */
static inline void ARGB_PrintPinConfig(uint32_t arduino_pin)
{
    ARGB_PinConfig_t cfg;
    ARGB_DMA_Result_t res = ARGB_AnalyzePin(arduino_pin, &cfg);
    
    Serial.print(F("Pin "));
    Serial.print(arduino_pin);
    Serial.print(F(": "));
    
    if (res != ARGB_DMA_OK) {
        Serial.println(ARGB_DMA_ResultString(res));
        return;
    }
    
    // GPIO Port name
    char port_name = '?';
    if (cfg.gpio_port == GPIOA) port_name = 'A';
    else if (cfg.gpio_port == GPIOB) port_name = 'B';
    else if (cfg.gpio_port == GPIOC) port_name = 'C';
    #ifdef GPIOD
    else if (cfg.gpio_port == GPIOD) port_name = 'D';
    #endif
    
    uint8_t pin_num = 0;
    uint32_t pin_mask = cfg.gpio_pin;
    while (pin_mask > 1) { pin_mask >>= 1; pin_num++; }
    
    Serial.print(F("P")); Serial.print(port_name); Serial.print(pin_num);
    
    // Timer
    uint8_t tim_num = 0;
    #ifdef TIM1
    if (cfg.tim == TIM1) tim_num = 1;
    #endif
    #ifdef TIM2
    if (cfg.tim == TIM2) tim_num = 2;
    #endif
    #ifdef TIM3
    if (cfg.tim == TIM3) tim_num = 3;
    #endif
    #ifdef TIM4
    if (cfg.tim == TIM4) tim_num = 4;
    #endif
    #ifdef TIM5
    if (cfg.tim == TIM5) tim_num = 5;
    #endif
    
    Serial.print(F(" -> TIM"));
    Serial.print(tim_num);
    
    uint8_t ch_num = 0;
    if (cfg.tim_channel == TIM_CHANNEL_1) ch_num = 1;
    else if (cfg.tim_channel == TIM_CHANNEL_2) ch_num = 2;
    else if (cfg.tim_channel == TIM_CHANNEL_3) ch_num = 3;
    else if (cfg.tim_channel == TIM_CHANNEL_4) ch_num = 4;
    
    Serial.print(F("_CH"));
    Serial.print(ch_num);
    Serial.print(cfg.is_32bit_tim ? F(" (32-bit)") : F(" (16-bit)"));
    
    // DMA
    uint8_t dma_num = 1;
    #ifdef DMA2
    if ((uint32_t)cfg.dma_stream >= (uint32_t)DMA2_Stream0) dma_num = 2;
    #endif
    
    uint8_t stream_num = 0;
    if (dma_num == 1) {
        stream_num = ((uint32_t)cfg.dma_stream - (uint32_t)DMA1_Stream0) / 0x18;
    }
    #ifdef DMA2
    else {
        stream_num = ((uint32_t)cfg.dma_stream - (uint32_t)DMA2_Stream0) / 0x18;
    }
    #endif
    
    uint8_t dma_ch_num = cfg.dma_channel >> 25;  // DMA_CHANNEL_x is (x << 25)
    
    Serial.print(F(" -> DMA"));
    Serial.print(dma_num);
    Serial.print(F("_Stream"));
    Serial.print(stream_num);
    Serial.print(F("/Ch"));
    Serial.println(dma_ch_num);
}

#else
// Pure C environment - no Serial print function
#endif // __cplusplus

#endif // ARGB_AUTO_H
