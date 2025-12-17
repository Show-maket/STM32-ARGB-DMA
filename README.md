# STM32-ARGB-DMA

**Arduino/STM32duino port** of the [STM32-ARGB-DMA](https://github.com/Crazy-Geeks/STM32-ARGB-DMA) library for addressable RGB LEDs (WS2812, WS2811, SK6812).

Uses **DMA** to drive LED strips without blocking the CPU or disabling interrupts.

## ✅ Tested Configuration

| Component | Version |
|-----------|---------|
| STM32duino Core | **2.8.1** |
| Board | Nucleo F401RE |
| LED Type | WS2812B |

## Features

- Non-blocking DMA transfer (CPU free during transmission)
- Supports WS2812/WS2811/SK6812 (RGB and RGBW)
- Auto-batching: multiple `Show()` calls are queued, not dropped
- HSV and RGB color models
- Automatic pin/timer/DMA configuration helpers

## Quick Start

### Automatic Configuration (Recommended)

```cpp
#include <ARGB.h>
#include <ARGB_Auto.h>

#define LED_PIN PA0
#define NUM_PIXELS 4

void setup() {
    // Check if pin supports PWM+DMA
    ARGB_PinConfig_t cfg;
    if (ARGB_AnalyzePin(LED_PIN, &cfg) != ARGB_DMA_OK) {
        Serial.println("Pin not supported!");
        return;
    }
    
    // ... setup TIM and DMA using cfg values ...
    // See examples for full code
}
```

### Manual Configuration (Full Control)

```cpp
#define NUM_PIXELS 4
#define WS2812
#define DMA_SIZE_WORD  // Required for TIM2/TIM5 (32-bit timers)
#include <ARGB.h>

TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma;

void setup() {
    // 1. Init GPIO for TIM2_CH1 on PA0
    // 2. Init TIM2 with HAL_TIM_PWM_Init()
    // 3. Init DMA1_Stream5_Channel3
    // 4. Call ARGB_Attach() and ARGB_Init()
}
```

## ⚠️ Critical Notes

### 32-bit Timers (TIM2, TIM5)

On STM32F4, TIM2 and TIM5 are 32-bit timers with 32-bit CCR registers.
**DMA MUST use WORD (32-bit) alignment**, otherwise CCR won't update correctly!

```cpp
// In your code BEFORE including ARGB.h:
#define DMA_SIZE_WORD

// In DMA init:
hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
```

### DMA Stream Conflicts

Each DMA stream can only be used by one peripheral. If your stream is already used
(e.g., by SPI, I2C, UART), use an alternate mapping:

| Timer | Channel | Primary Stream | Alternate Stream |
|-------|---------|----------------|------------------|
| TIM2  | CH1     | DMA1_Stream5   | DMA1_Stream1     |
| TIM2  | CH4     | DMA1_Stream7   | DMA1_Stream6     |
| TIM5  | CH4     | DMA1_Stream3   | DMA1_Stream1     |

### Logic Levels

STM32 outputs 3.3V but WS2812 expects VIH > 0.7×VDD (3.5V at 5V supply).
For reliable operation:
- Use level shifter (SN74LVC1G125), OR
- Use 330Ω series resistor + 4.5V LED supply, OR
- Use open-drain with 1K pull-up to 5V

## API Reference

### Core Functions

```cpp
ARGB_Attach(&htim, channel, &hdma, timer_clock_hz); // Bind to timer/DMA
ARGB_Init();                          // Initialize timing
ARGB_SetBrightness(0-255);            // Global brightness
ARGB_SetRGB(pixel, r, g, b);          // Set pixel color
ARGB_SetHSV(pixel, h, s, v);          // Set pixel HSV
ARGB_FillRGB(r, g, b);                // Fill all pixels
ARGB_Clear();                         // Turn off all pixels
ARGB_Show();                          // Send to strip (non-blocking)
ARGB_Ready();                         // Check if ready for new frame
```

### Auto-Configuration (ARGB_Auto.h)

```cpp
ARGB_AnalyzePin(pin, &config);        // Get TIM/DMA config for pin
ARGB_IsPinSupported(pin);             // Quick check
ARGB_PrintPinConfig(pin);             // Debug output to Serial
```

## Memory Usage

| Pixels | PWM Buffer | RGB Buffer | Total |
|--------|------------|------------|-------|
| 4      | 576 bytes  | 12 bytes   | ~600 B |
| 16     | 1.6 KB     | 48 bytes   | ~1.7 KB |
| 60     | 5.8 KB     | 180 bytes  | ~6 KB |
| 144    | 14 KB      | 432 bytes  | ~14.5 KB |

Formula: `PWM_BUF = (NUM_PIXELS × 24 + 48) × 4 bytes`

## DMA Mapping Tables

Currently supported:
- ✅ STM32F4xx (F401, F405, F407, F411, F446, etc.)
- ⏳ STM32F1xx (planned)
- ⏳ STM32G4xx (planned)

To request support for other chips, open an issue with:
1. Chip model (e.g., STM32G431)
2. DMA request mapping table from Reference Manual

## Troubleshooting

### "Pin doesn't support PWM"
- Check that pin has TIM alternate function (see datasheet)
- Some pins support multiple timers - try `_ALT1` variant

### No signal / Constant HIGH
- Verify DMA stream is correct for your TIM channel
- Check DMA data alignment (WORD for TIM2/TIM5)
- Ensure GPIO is in AF mode with correct AF number

### Signal looks correct but LEDs don't work
- Check logic levels (3.3V vs 5V)
- Verify timing with oscilloscope (T0H ~350ns, T1H ~700ns)
- Try shorter wires / add 100-470Ω series resistor

### DMA conflicts
- Use `ARGB_DMA_Map_F4_Alt[]` for alternate stream
- Or manually configure with different stream

## License

MIT License - see LICENSE file

## Credits

- **Original library**: [Crazy-Geeks/STM32-ARGB-DMA](https://github.com/Crazy-Geeks/STM32-ARGB-DMA) by Dmitriy Semenov (Crazy_Geeks)
- **Arduino/STM32duino port**: DashyFox
  - Runtime TIM/DMA binding (`ARGB_Attach()`)
  - Automatic pin configuration (`ARGB_Auto.h`)
  - Arduino-friendly API (`ARGB_Begin()`)
