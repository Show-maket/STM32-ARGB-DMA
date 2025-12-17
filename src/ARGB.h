/**
 *******************************************
 * @file    ARGB.h
 * @author  Dmitriy Semenov / Crazy_Geeks
 * @link    https://crazygeeks.ru
 * @version 1.33
 * @date	17-May-2022
 * @brief   Header file for ARGB Driver (Addressable RGB)
 *******************************************
 *
 * @note Repo: https://github.com/Crazy-Geeks/STM32-ARGB-DMA
 * @note RU article: https://crazygeeks.ru/stm32-argb-lib
 * 
 * @note Arduino/STM32duino port by DashyFox:
 *       - Added ARGB_Attach() for runtime TIM/DMA binding
 */

#ifndef ARGB_H_
#define ARGB_H_

#include "libs.h"

/**
 * @addtogroup ARGB_Driver
 * @brief Addressable RGB LED Driver
 * @{
 * @addtogroup User_settings
 * @brief LED & Timer's settings
 * @{
 */

// ---- Strip family selection (define before including ARGB.h) ----
#if !defined(WS2811S) && !defined(WS2811F) && !defined(WS2812) && !defined(SK6812)
#define WS2812       ///< Default: WS2812 (GRB, 800kHz)
#endif
// WS2811S — RGB, 400kHz;
// WS2811F — RGB, 800kHz;
// WS2812  — GRB, 800kHz;
// SK6812  — RGBW, 800kHz

#ifndef NUM_PIXELS
#define NUM_PIXELS 5 ///< Pixel quantity (define before including)
#endif

#ifndef USE_GAMMA_CORRECTION
#define USE_GAMMA_CORRECTION 0 ///< Gamma-correction (0/1)
#endif

// Legacy CubeMX settings (not used with ARGB_Auto.h)
#ifndef TIM_NUM
#define TIM_NUM	   2  ///< Timer number
#endif
#ifndef TIM_CH
#define TIM_CH	   TIM_CHANNEL_2  ///< Timer's PWM channel
#endif
#ifndef DMA_HANDLE
#define DMA_HANDLE hdma_tim2_ch2_ch4  ///< DMA Channel
#endif
#if !defined(DMA_SIZE_BYTE) && !defined(DMA_SIZE_HWORD) && !defined(DMA_SIZE_WORD)
#define DMA_SIZE_WORD     ///< DMA Data Width (default WORD for 32-bit timers)
#endif

/// @}

/**
 * @addtogroup Global_entities
 * @brief All driver's methods
 * @{
 * @enum ARGB_STATE
 * @brief Driver's status enum
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum ARGB_STATE {
    ARGB_BUSY = 0,  ///< DMA Transfer in progress
    ARGB_READY = 1, ///< DMA Ready to transfer
    ARGB_OK = 2,    ///< Function execution success
    ARGB_PARAM_ERR = 3, ///< Error in input parameters
} ARGB_STATE;

void ARGB_Init(void);   // Initialization
void ARGB_Clear(void);  // Clear strip

void ARGB_SetBrightness(u8_t br); // Set global brightness

void ARGB_SetRGB(u16_t i, u8_t r, u8_t g, u8_t b);  // Set single LED by RGB
void ARGB_SetHSV(u16_t i, u8_t hue, u8_t sat, u8_t val); // Set single LED by HSV
void ARGB_SetWhite(u16_t i, u8_t w); // Set white component in LED (RGBW)

void ARGB_FillRGB(u8_t r, u8_t g, u8_t b); // Fill all strip with RGB color
void ARGB_FillHSV(u8_t hue, u8_t sat, u8_t val); // Fill all strip with HSV color
void ARGB_FillWhite(u8_t w); // Fill all strip's white component (RGBW)

ARGB_STATE ARGB_Ready(void); // Get DMA Ready state
ARGB_STATE ARGB_Show(void); // Push data to the strip

/**
 * @brief  Runtime binding to TIM/DMA (Arduino/STM32duino friendly)
 * @note   Added by DashyFox for Arduino/STM32duino port
 */
ARGB_STATE ARGB_Attach(TIM_HandleTypeDef* htim,
                       u32_t tim_channel,
                       DMA_HandleTypeDef* hdma,
                       u32_t timer_clock_hz);

#ifdef __cplusplus
}
#endif

/// @} @}
#endif /* ARGB_H_ */