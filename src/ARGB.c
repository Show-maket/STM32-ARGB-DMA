/**
 *******************************************
 * @file    ARGB.c
 * @author  Dmitriy Semenov / Crazy_Geeks
 * @link    https://crazygeeks.ru
 * @version 1.33
 * @date	17-May-2022
 * @brief   Source file for ARGB Driver (Addressable RGB)
 *******************************************
 *
 * @note Repo: https://github.com/Crazy-Geeks/STM32-ARGB-DMA
 * @note RU article: https://crazygeeks.ru/stm32-argb-lib
 * 
 * @note Arduino/STM32duino port by DashyFox:
 *       - Added ARGB_Attach() for runtime TIM/DMA binding
 *       - Added conditional compilation for Arduino support
 */

/* WS2811 Timings
 * Tolerance: +/- 150ns <-> +/- 0.15us
 * RES: >50us
 *
 * Slow mode:
 * Period: 2.5us <-> 400 KHz
 * T0H: 0.5us
 * T1H: 1.2us
 * T0L: 2.0us
 * T1L: 1.3us
 *
 * Fast mode:
 * Period: 1.25us <-> 800 KHz
 * T0H: 0.25us - 20%
 * T1H: 0.6us  - 48%
 * T0L: 1.0us
 * T1H: 0.65us
 *
 */

/* WS2811 Timings
 * Tolerance: +/- 150ns <-> +/- 0.15us
 * RES: >50us

 * Period: 1.25us <-> 800 KHz
 * T0H: 0.35us - 20%
 * T1H: 0.7us  - 48%
 * T0L: 0.8us
 * T1H: 0.6us
 *
 */

#include "ARGB.h"  // include header file
#include "math.h"

/**
 * @addtogroup ARGB_Driver
 * @{
 */

/**
 * @addtogroup Private_entities
 * @brief Private methods and variables
 * @{
*/

/// Timer handler
#if TIM_NUM == 1
#define TIM_HANDLE  htim1
#elif TIM_NUM == 2
#define TIM_HANDLE  htim2
#elif TIM_NUM == 3
#define TIM_HANDLE  htim3
#elif TIM_NUM == 4
#define TIM_HANDLE  htim4
#elif TIM_NUM == 5
#define TIM_HANDLE  htim5
#elif TIM_NUM == 8
#define TIM_HANDLE  htim8
#else
#error Wrong timer! Fix it in ARGB.h string 41
#warning If you shure, set TIM_HANDLE and APB ring by yourself
#endif

/// Timer's RCC Bus
#if TIM_NUM == 1 || (TIM_NUM >= 8 && TIM_NUM <= 11)
#define APB1
#else
#define APB2
#endif

/// DMA Size
#if defined(DMA_SIZE_BYTE)
typedef u8_t dma_siz;
#elif defined(DMA_SIZE_HWORD)
typedef u16_t dma_siz;
#elif defined(DMA_SIZE_WORD)
typedef u32_t dma_siz;
#endif

// For Arduino/STM32duino: use runtime binding only (no CubeMX externs)
#if defined(ARDUINO) || defined(ARDUINO_ARCH_STM32)
#define ARGB_USE_RUNTIME_BINDING
#endif

// Legacy CubeMX-style externs (when not using ARGB_Attach)
#ifndef ARGB_USE_RUNTIME_BINDING
extern TIM_HandleTypeDef (TIM_HANDLE);  ///< Timer handler
extern DMA_HandleTypeDef (DMA_HANDLE);  ///< DMA handler
#endif

// Runtime binding pointers (set by ARGB_Attach)
static TIM_HandleTypeDef* s_htim = NULL;
static DMA_HandleTypeDef* s_hdma = NULL;
static u32_t s_tim_channel = 0;
static volatile u32_t* s_tim_ccr = NULL;
static u32_t s_tim_dma_cc = 0;
static u32_t s_timer_clock_hz = 0;

volatile u8_t PWM_HI;    ///< PWM Code HI Log.1 period
volatile u8_t PWM_LO;    ///< PWM Code LO Log.1 period

#ifdef SK6812
#define NUM_BYTES (4 * NUM_PIXELS)          ///< Strip size in bytes (RGBW)
#define BITS_PER_PIXEL (4 * 8)              ///< 32 bits per RGBW pixel
#else
#define NUM_BYTES (3 * NUM_PIXELS)          ///< Strip size in bytes (RGB)
#define BITS_PER_PIXEL (3 * 8)              ///< 24 bits per RGB pixel
#endif

#define RST_LEN 60                          ///< Reset period (60+ bits of LOW = 75us @ 800kHz)
#define PWM_BUF_LEN (NUM_PIXELS * BITS_PER_PIXEL + RST_LEN)  ///< Full buffer for all pixels + reset

/// Static LED buffer
volatile u8_t RGB_BUF[NUM_BYTES] = {0,};

/// Timer PWM value buffer - holds ALL data for complete DMA transfer
volatile dma_siz PWM_BUF[PWM_BUF_LEN] = {0,};

volatile u8_t ARGB_BR = 255;     ///< LED Global brightness
volatile ARGB_STATE ARGB_LOC_ST; ///< Buffer send status

static inline u8_t scale8(u8_t x, u8_t scale); // Gamma correction
static void HSV2RGB(u8_t hue, u8_t sat, u8_t val, u8_t *_r, u8_t *_g, u8_t *_b);
// Callbacks
static void ARGB_TIM_DMADelayPulseCplt(DMA_HandleTypeDef *hdma);
static void ARGB_TIM_DMADelayPulseHalfCplt(DMA_HandleTypeDef *hdma);
/// @} //Private

/**
 * @brief Runtime binding to TIM/DMA (Arduino-friendly)
 * @note Added by DashyFox for Arduino/STM32duino port
 *       Allows dynamic binding instead of compile-time extern declarations
 */
ARGB_STATE ARGB_Attach(TIM_HandleTypeDef* htim, u32_t tim_channel,
                       DMA_HandleTypeDef* hdma, u32_t timer_clock_hz) {
    if (!htim || !hdma) return ARGB_PARAM_ERR;
    
    s_htim = htim;
    s_hdma = hdma;
    s_tim_channel = tim_channel;
    s_timer_clock_hz = timer_clock_hz;
    
    // Get CCR register address
    switch (tim_channel) {
        case TIM_CHANNEL_1: s_tim_ccr = &htim->Instance->CCR1; s_tim_dma_cc = TIM_DMA_CC1; break;
        case TIM_CHANNEL_2: s_tim_ccr = &htim->Instance->CCR2; s_tim_dma_cc = TIM_DMA_CC2; break;
        case TIM_CHANNEL_3: s_tim_ccr = &htim->Instance->CCR3; s_tim_dma_cc = TIM_DMA_CC3; break;
        case TIM_CHANNEL_4: s_tim_ccr = &htim->Instance->CCR4; s_tim_dma_cc = TIM_DMA_CC4; break;
        default: return ARGB_PARAM_ERR;
    }
    return ARGB_OK;
}

/**
 * @brief Init timer & prescalers
 * @param none
 */
void ARGB_Init(void) {
    /* Auto-calculation! */
    u32_t APBfq; // Clock freq
    
    // Use runtime binding if available, otherwise legacy defines
    TIM_TypeDef* tim_inst;
    u32_t tim_ch;
    
    if (s_htim != NULL) {
        // Runtime binding mode (ARGB_Attach was called)
        tim_inst = s_htim->Instance;
        tim_ch = s_tim_channel;
        APBfq = s_timer_clock_hz;
    } else {
#ifndef ARGB_USE_RUNTIME_BINDING
        // Legacy CubeMX mode
        tim_inst = TIM_HANDLE.Instance;
        tim_ch = TIM_CH;
#ifdef APB1
        APBfq = HAL_RCC_GetPCLK1Freq();
        APBfq *= (RCC->CFGR & RCC_CFGR_PPRE1) == 0 ? 1 : 2;
#else
        APBfq = HAL_RCC_GetPCLK2Freq();
        APBfq *= (RCC->CFGR & RCC_CFGR_PPRE2) == 0 ? 1 : 2;
#endif
#else
        return; // No timer configured
#endif
    }

#ifdef WS2811S
    APBfq /= (uint32_t) (400 * 1000);  // 400 KHz - 2.5us
#else
    APBfq /= (uint32_t) (800 * 1000);  // 800 KHz - 1.25us
#endif
    tim_inst->PSC = 0;                        // no prescaler
    tim_inst->ARR = (uint16_t) (APBfq - 1);   // set timer period
    tim_inst->EGR = 1;                        // update registers
    
#if defined(WS2811F) || defined(WS2811S)
    PWM_HI = (u8_t) (APBfq * 0.48) - 1;     // Log.1 - 48% - 0.60us/1.2us
    PWM_LO = (u8_t) (APBfq * 0.20) - 1;     // Log.0 - 20% - 0.25us/0.5us
#endif
#ifdef WS2812
    PWM_HI = (u8_t) (APBfq * 0.56) - 1;     // Log.1 - 56% - 0.70us
    PWM_LO = (u8_t) (APBfq * 0.28) - 1;     // Log.0 - 28% - 0.35us
#endif
#ifdef SK6812
    PWM_HI = (u8_t) (APBfq * 0.48) - 1;     // Log.1 - 48% - 0.60us
    PWM_LO = (u8_t) (APBfq * 0.24) - 1;     // Log.0 - 24% - 0.30us
#endif

    ARGB_LOC_ST = ARGB_READY; // Set Ready Flag
    TIM_CCxChannelCmd(tim_inst, tim_ch, TIM_CCx_ENABLE); // Enable GPIO to IDLE state
    HAL_Delay(1); // Make some delay
}

/**
 * @brief Fill ALL LEDs with (0,0,0)
 * @param none
 * @note Update strip after that
 */
void ARGB_Clear(void) {
    ARGB_FillRGB(0, 0, 0);
#ifdef SK6812
    ARGB_FillWhite(0);
#endif
}

/**
 * @brief Set GLOBAL LED brightness
 * @param[in] br Brightness [0..255]
 */
void ARGB_SetBrightness(u8_t br) {
    ARGB_BR = br;
}

/**
 * @brief Set LED with RGB color by index
 * @param[in] i LED position
 * @param[in] r Red component   [0..255]
 * @param[in] g Green component [0..255]
 * @param[in] b Blue component  [0..255]
 */
void ARGB_SetRGB(u16_t i, u8_t r, u8_t g, u8_t b) {
    // overflow protection
    if (i >= NUM_PIXELS) {
        u16_t _i = i / NUM_PIXELS;
        i -= _i * NUM_PIXELS;
    }
    // set brightness
    r /= 256 / ((u16_t) ARGB_BR + 1);
    g /= 256 / ((u16_t) ARGB_BR + 1);
    b /= 256 / ((u16_t) ARGB_BR + 1);
#if USE_GAMMA_CORRECTION
    g = scale8(g, 0xB0);
    b = scale8(b, 0xF0);
#endif
    // Subpixel chain order
#if defined(SK6812) || defined(WS2811F) || defined(WS2811S)
    const u8_t subp1 = r;
    const u8_t subp2 = g;
    const u8_t subp3 = b;
#else
    const u8_t subp1 = g;
    const u8_t subp2 = r;
    const u8_t subp3 = b;
#endif
    // RGB or RGBW
#ifdef SK6812
    RGB_BUF[4 * i] = subp1;     // subpixel 1
    RGB_BUF[4 * i + 1] = subp2; // subpixel 2
    RGB_BUF[4 * i + 2] = subp3; // subpixel 3
#else
    RGB_BUF[3 * i] = subp1;     // subpixel 1
    RGB_BUF[3 * i + 1] = subp2; // subpixel 2
    RGB_BUF[3 * i + 2] = subp3; // subpixel 3
#endif
}

/**
 * @brief Set LED with HSV color by index
 * @param[in] i LED position
 * @param[in] hue HUE (color) [0..255]
 * @param[in] sat Saturation  [0..255]
 * @param[in] val Value (brightness) [0..255]
 */
void ARGB_SetHSV(u16_t i, u8_t hue, u8_t sat, u8_t val) {
    uint8_t _r, _g, _b;                    // init buffer color
    HSV2RGB(hue, sat, val, &_r, &_g, &_b); // get RGB color
    ARGB_SetRGB(i, _r, _g, _b);     // set color
}

/**
 * @brief Set White component in strip by index
 * @param[in] i LED position
 * @param[in] w White component [0..255]
 */
void ARGB_SetWhite(u16_t i, u8_t w) {
#ifdef RGB
    return;
#endif
    w /= 256 / ((u16_t) ARGB_BR + 1); // set brightness
    RGB_BUF[4 * i + 3] = w;                // set white part
}

/**
 * @brief Fill ALL LEDs with RGB color
 * @param[in] r Red component   [0..255]
 * @param[in] g Green component [0..255]
 * @param[in] b Blue component  [0..255]
 */
void ARGB_FillRGB(u8_t r, u8_t g, u8_t b) {
    for (volatile u16_t i = 0; i < NUM_PIXELS; i++)
        ARGB_SetRGB(i, r, g, b);
}

/**
 * @brief Fill ALL LEDs with HSV color
 * @param[in] hue HUE (color) [0..255]
 * @param[in] sat Saturation  [0..255]
 * @param[in] val Value (brightness) [0..255]
 */
void ARGB_FillHSV(u8_t hue, u8_t sat, u8_t val) {
    uint8_t _r, _g, _b;                    // init buffer color
    HSV2RGB(hue, sat, val, &_r, &_g, &_b); // get color once (!)
    ARGB_FillRGB(_r, _g, _b);       // set color
}

/**
 * @brief Set ALL White components in strip
 * @param[in] w White component [0..255]
 */
void ARGB_FillWhite(u8_t w) {
    for (volatile u16_t i = 0; i < NUM_PIXELS; i++)
        ARGB_SetWhite(i, w);
}

/**
 * @brief Get current DMA status
 * @param none
 * @return #ARGB_STATE enum
 */
ARGB_STATE ARGB_Ready(void) {
    return ARGB_LOC_ST;
}

/**
 * @brief Update strip - fills entire PWM buffer and starts single DMA transfer
 * @param none
 * @return #ARGB_STATE enum
 */
ARGB_STATE ARGB_Show(void) {
    // Get handles - runtime or legacy
    TIM_HandleTypeDef* htim;
    DMA_HandleTypeDef* hdma;
    u32_t tim_ch, tim_dma_cc;
    volatile u32_t* ccr_reg;
    
    if (s_htim != NULL) {
        htim = s_htim;
        hdma = s_hdma;
        tim_ch = s_tim_channel;
        tim_dma_cc = s_tim_dma_cc;
        ccr_reg = s_tim_ccr;
    } else {
#ifndef ARGB_USE_RUNTIME_BINDING
        htim = &TIM_HANDLE;
        hdma = &DMA_HANDLE;
        tim_ch = TIM_CH;
#if TIM_CH == TIM_CHANNEL_1
        tim_dma_cc = TIM_DMA_CC1;
        ccr_reg = &htim->Instance->CCR1;
#elif TIM_CH == TIM_CHANNEL_2
        tim_dma_cc = TIM_DMA_CC2;
        ccr_reg = &htim->Instance->CCR2;
#elif TIM_CH == TIM_CHANNEL_3
        tim_dma_cc = TIM_DMA_CC3;
        ccr_reg = &htim->Instance->CCR3;
#else
        tim_dma_cc = TIM_DMA_CC4;
        ccr_reg = &htim->Instance->CCR4;
#endif
#else
        return ARGB_PARAM_ERR;
#endif
    }

    // Check if DMA is ready
    if (ARGB_LOC_ST == ARGB_BUSY || hdma->State != HAL_DMA_STATE_READY) {
        return ARGB_BUSY;
    }
    ARGB_LOC_ST = ARGB_BUSY;
    
    // Fill ENTIRE PWM buffer with all pixel data
    u16_t pwm_idx = 0;
    for (u16_t byte_idx = 0; byte_idx < NUM_BYTES; byte_idx++) {
        u8_t byte_val = RGB_BUF[byte_idx];
        for (u8_t bit = 0; bit < 8; bit++) {
            PWM_BUF[pwm_idx++] = (byte_val & 0x80) ? PWM_HI : PWM_LO;
            byte_val <<= 1;
        }
    }
    
    // Add reset period (zeros for LOW signal)
    for (u16_t i = 0; i < RST_LEN; i++) {
        PWM_BUF[pwm_idx++] = 0;
    }
    
    // Clear CCR before starting to avoid initial glitch
    *ccr_reg = 0;
    htim->Instance->CNT = 0;
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE | TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC3 | TIM_FLAG_CC4);
    
    // Get DMA ID from channel
    u32_t dma_id = (tim_ch == TIM_CHANNEL_1) ? TIM_DMA_ID_CC1 :
                   (tim_ch == TIM_CHANNEL_2) ? TIM_DMA_ID_CC2 :
                   (tim_ch == TIM_CHANNEL_3) ? TIM_DMA_ID_CC3 : TIM_DMA_ID_CC4;
    
    // Set channel state
    TIM_CHANNEL_STATE_SET(htim, tim_ch, HAL_TIM_CHANNEL_STATE_BUSY);
    
    // Setup callbacks
    htim->hdma[dma_id]->XferCpltCallback = ARGB_TIM_DMADelayPulseCplt;
    htim->hdma[dma_id]->XferHalfCpltCallback = NULL;  // Not needed for NORMAL mode
    htim->hdma[dma_id]->XferErrorCallback = TIM_DMAError;
    
    // Start DMA for entire buffer
    if (HAL_DMA_Start_IT(htim->hdma[dma_id], (u32_t)PWM_BUF,
                         (u32_t)ccr_reg, (u16_t)PWM_BUF_LEN) != HAL_OK) {
        ARGB_LOC_ST = ARGB_READY;
        TIM_CHANNEL_STATE_SET(htim, tim_ch, HAL_TIM_CHANNEL_STATE_READY);
        return ARGB_PARAM_ERR;
    }
    
    // Enable DMA request from timer
    __HAL_TIM_ENABLE_DMA(htim, tim_dma_cc);
    
    // Enable timer output for advanced timers (TIM1, TIM8)
    if (IS_TIM_BREAK_INSTANCE(htim->Instance) != RESET)
        __HAL_TIM_MOE_ENABLE(htim);
    
    // Start timer
    __HAL_TIM_ENABLE(htim);
    
    return ARGB_OK;
}

/**
 * @addtogroup Private_entities
 * @{ */

/**
 * @brief Private method for gamma correction
 * @param[in] x Param to scale
 * @param[in] scale Scale coefficient
 * @return Scaled value
 */
static inline u8_t scale8(u8_t x, u8_t scale) {
    return ((uint16_t) x * scale) >> 8;
}

/**
 * @brief Convert color in HSV to RGB
 * @param[in] hue HUE (color) [0..255]
 * @param[in] sat Saturation  [0..255]
 * @param[in] val Value (brightness) [0..255]
 * @param[out] _r Pointer to RED component value
 * @param[out] _g Pointer to GREEN component value
 * @param[out] _b Pointer to BLUE component value
 */
static void HSV2RGB(u8_t hue, u8_t sat, u8_t val, u8_t *_r, u8_t *_g, u8_t *_b) {
    if (sat == 0) { // if white color
        *_r = *_g = *_b = val;
        return;
    }
    // Float is smoother but check for FPU (Floating point unit) in your MCU
    // Otherwise it will take longer time in the code
    // FPU is in: F3/L3 and greater
    // Src: https://github.com/Inseckto/HSV-to-RGB
    float h = (float)hue / 255;
    float s = (float)sat / 255;
    float v = (float)val / 255;

    int i = (int)floorf(h * 6);
    float f = h * 6 - (float)i;
    u8_t p = (u8_t)(v * (1 - s) * 255.0);
    u8_t q = (u8_t)(v * (1 - f * s) * 255.0);
    u8_t t = (u8_t)(v * (1 - (1 - f) * s)*255.0);

    switch (i % 6) {
// Src: https://stackoverflow.com/questions/3018313
//    uint8_t reg = hue / 43;
//    uint8_t rem = (hue - (reg * 43)) * 6;
//    uint8_t p = (val * (255 - sat)) >> 8;
//    uint8_t q = (val * (255 - ((sat * rem) >> 8))) >> 8;
//    uint8_t t = (val * (255 - ((sat * (255 - rem)) >> 8))) >> 8;
//    switch (reg) {
        case 0: *_r = val, *_g = t, *_b = p; break;
        case 1: *_r = q, *_g = val, *_b = p; break;
        case 2: *_r = p, *_g = val, *_b = t; break;
        case 3: *_r = p, *_g = q, *_b = val; break;
        case 4: *_r = t, *_g = p, *_b = val; break;
        default: *_r = val, *_g = p, *_b = q; break;
    }
}

/**
  * @brief  TIM DMA Delay Pulse complete callback (NORMAL mode).
  *         Called when entire PWM buffer has been transmitted.
  * @param  hdma pointer to DMA handle.
  * @retval None
  */
static void ARGB_TIM_DMADelayPulseCplt(DMA_HandleTypeDef *hdma) {
    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *) hdma->Parent;
    
    // Get channel info
    u32_t tim_ch, tim_dma_cc;
    if (s_htim != NULL) {
        tim_ch = s_tim_channel;
        tim_dma_cc = s_tim_dma_cc;
    } else {
#ifndef ARGB_USE_RUNTIME_BINDING
        tim_ch = TIM_CH;
        tim_dma_cc = (TIM_CH == TIM_CHANNEL_1) ? TIM_DMA_CC1 :
                     (TIM_CH == TIM_CHANNEL_2) ? TIM_DMA_CC2 :
                     (TIM_CH == TIM_CHANNEL_3) ? TIM_DMA_CC3 : TIM_DMA_CC4;
#else
        return;  // No valid configuration
#endif
    }
    
    // Stop DMA and timer
    __HAL_TIM_DISABLE_DMA(htim, tim_dma_cc);
    
    if (IS_TIM_BREAK_INSTANCE(htim->Instance) != RESET)
        __HAL_TIM_MOE_DISABLE(htim);
    __HAL_TIM_DISABLE(htim);
    
    // Set channel state to ready
    TIM_CHANNEL_STATE_SET(htim, tim_ch, HAL_TIM_CHANNEL_STATE_READY);
    
    // Clear active channel
    htim->Channel = HAL_TIM_ACTIVE_CHANNEL_CLEARED;
    
    // Signal completion
    ARGB_LOC_ST = ARGB_READY;
}

/**
  * @brief  TIM DMA Delay Pulse half complete callback (unused in NORMAL mode).
  * @param  hdma pointer to DMA handle.
  * @retval None
  */
static void ARGB_TIM_DMADelayPulseHalfCplt(DMA_HandleTypeDef *hdma) {
    (void)hdma;  // Unused in NORMAL mode
}

/** @} */ // Private

/** @} */ // Driver

// Check strip type
#if !(defined(SK6812) || defined(WS2811F) || defined(WS2811S) || defined(WS2812))
#error INCORRECT LED TYPE
#warning Set it from list in ARGB.h string 29
#endif

// Check channel (only for legacy CubeMX mode)
#ifndef ARGB_USE_RUNTIME_BINDING
#if !(TIM_CH == TIM_CHANNEL_1 || TIM_CH == TIM_CHANNEL_2 || TIM_CH == TIM_CHANNEL_3 || TIM_CH == TIM_CHANNEL_4)
#error Wrong channel! Fix it in ARGB.h string 40
#endif
#endif

// Check DMA Size
#if !(defined(DMA_SIZE_BYTE) | defined(DMA_SIZE_HWORD) | defined(DMA_SIZE_WORD))
#error Wrong DMA Size! Fix it in ARGB.h string 42
#endif
