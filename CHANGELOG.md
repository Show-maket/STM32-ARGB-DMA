# Changelog

All notable changes to this project will be documented in this file.

## [1.34.0-arduino-fork] - Arduino/STM32duino Port

### Added
- **Runtime TIM/DMA binding** (`ARGB_Attach()`) - allows dynamic configuration without compile-time extern declarations
- **Automatic pin configuration** (`ARGB_Auto.h`) - automatically detects TIM/DMA mapping for any PWM pin
- **Arduino-friendly API** (`ARGB_Begin()`) - one-line initialization for Arduino sketches
- **Arduino/STM32duino support** - conditional compilation for Arduino environment

### Changed
- Modified `ARGB.c` and `ARGB.h` to support runtime binding alongside legacy CubeMX mode
- Updated `libs.h` to use `stm32_def.h` for Arduino instead of CubeMX `main.h`

### Contributors
- **DashyFox** - Arduino/STM32duino port and enhancements

---

## [1.33] - Original Release

Original library by Dmitriy Semenov (Crazy_Geeks)
- Initial CubeMX-based implementation
- DMA + PWM driver for WS2812/WS2811/SK6812 LEDs

