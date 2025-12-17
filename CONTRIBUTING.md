# Руководство по расширению STM32-ARGB-DMA

Это руководство описывает, как добавить поддержку нового семейства STM32, новых таймеров или светодиодных чипов.

## Содержание

1. [Архитектура библиотеки](#архитектура-библиотеки)
2. [Добавление нового LED-чипа](#добавление-нового-led-чипа)
3. [Добавление нового семейства STM32](#добавление-нового-семейства-stm32)
4. [Настройка DMA для конкретного таймера](#настройка-dma-для-конкретного-таймера)
5. [Чеклист при добавлении нового чипа](#чеклист-при-добавлении-нового-чипа)
6. [Отладка](#отладка)

---

## Архитектура библиотеки

### Основные файлы

```
Library/
├── ARGB.h         # Главный заголовочный файл, настройки
├── ARGB.c         # Реализация: буферы, DMA управление
├── ARGB_Auto.h    # Автоанализ пинов (опционально)
└── README.md      # Документация API
```

### Ключевые компоненты

1. **PWM буфер** (`PWM_BUF[]`) - массив значений CCR для DMA
2. **Тайминги** (`PWM_HI`, `PWM_LO`) - значения CCR для бита 1 и 0
3. **DMA Transfer** - одиночная передача всего буфера в NORMAL режиме
4. **Batching** (`ARGB_PENDING_SEND`) - очередь для отложенных вызовов Show()

### Поток данных

```
ARGB_SetRGB() → RGB_BUF[] → ARGB_Show() → PWM_BUF[] → DMA → TIM_CCRx → GPIO
```

---

## Добавление нового LED-чипа

Чтобы добавить поддержку нового типа светодиода (например, APA102, WS2815):

### 1. Откройте `ARGB.h`

### 2. Добавьте определение чипа в секцию "LED Type selection"

```c
// В секции #ifdef/#elif для выбора чипа
#elif defined(WS2815)
    #define T0H     300     // Время HIGH для бита 0 (в наносекундах)
    #define T1H     750     // Время HIGH для бита 1 (в наносекундах)
    #define T0L     750     // Время LOW для бита 0 (нс) - обычно рассчитывается
    #define T1L     300     // Время LOW для бита 1 (нс)
    #define RES     280000  // Время RESET (в наносекундах) - 280 мкс для WS2815
```

### 3. Определите формат данных

Если новый чип использует другой порядок цветов (не GRB):

```c
// В ARGB.h
#elif defined(MY_NEW_LED)
    // Тайминги...
    #define LED_ORDER_RGB  // или LED_ORDER_GRB, LED_ORDER_BGR
```

Затем в `ARGB.c` добавьте обработку в функциях `ARGB_SetRGB()` и `ARGB_FillRGB()`:

```c
#ifdef LED_ORDER_RGB
    RGB_BUF[i].r = r;
    RGB_BUF[i].g = g;
    RGB_BUF[i].b = b;
#else // GRB (default)
    RGB_BUF[i].g = g;
    RGB_BUF[i].r = r;
    RGB_BUF[i].b = b;
#endif
```

### 4. Для RGBW светодиодов (SK6812W, etc.)

Добавьте определение `ARGB_USE_RGBW`:

```c
#elif defined(SK6812W)
    #define T0H     300
    #define T1H     600
    #define RES     80000
    #define ARGB_USE_RGBW  // 4 канала вместо 3
```

---

## Добавление нового семейства STM32

### 1. Откройте `ARGB_Auto.h`

### 2. Добавьте таблицу DMA маппинга

Найдите секцию `#if defined(STM32F4xx)` и добавьте аналогичную для нового семейства:

```c
#elif defined(STM32G0xx)
// STM32G0 использует DMAMUX, маппинг отличается!
typedef struct {
    TIM_TypeDef*        tim;
    uint32_t            channel;
    DMA_TypeDef*        dma;
    DMA_Channel_TypeDef* dma_channel;  // Для G0 - Channel, не Stream!
    uint32_t            dmamux_req;    // DMAMUX request ID
    IRQn_Type           irqn;
} ARGB_DMA_Map_G0_t;

static const ARGB_DMA_Map_G0_t ARGB_DMA_MAP_G0[] = {
    // Заполняется из Reference Manual (таблица DMAMUX requests)
    {TIM1, TIM_CHANNEL_1, DMA1, DMA1_Channel1, DMA_REQUEST_TIM1_CH1, DMA1_Channel1_IRQn},
    // ...
    {NULL, 0, NULL, NULL, 0, (IRQn_Type)0}
};
#endif
```

### 3. Где взять таблицу маппинга

- **Reference Manual** → раздел "DMA request mapping" или "DMAMUX"
- Например, для STM32F4: RM0090, Table 28 "DMA1 request mapping"
- Для STM32G0/G4/H7: RM главы про DMAMUX

### 4. Обновите функцию `ARGB_AnalyzePin()`

Добавьте обработку нового семейства:

```c
#elif defined(STM32G0xx)
    const ARGB_DMA_Map_G0_t* dma_map = ARGB_DMA_MAP_G0;
    while (dma_map->tim != NULL) {
        if (dma_map->tim == cfg->tim && dma_map->channel == cfg->tim_channel) {
            cfg->dma_channel_handle = dma_map->dma_channel;
            cfg->dmamux_request = dma_map->dmamux_req;
            cfg->dma_irqn = dma_map->irqn;
            return ARGB_DMA_OK;
        }
        dma_map++;
    }
#endif
```

### 5. Особенности разных семейств

| Семейство | DMA тип | Особенности |
|-----------|---------|-------------|
| F1, F0, L0, L1 | Channel-based | Без streams, фиксированный маппинг |
| F2, F4, F7 | Stream-based | 8 streams × 8 channels |
| G0, G4, L4+, H7 | DMAMUX | Гибкий маппинг через мультиплексор |

---

## Настройка DMA для конкретного таймера

### Критичные параметры

#### 1. Разрядность таймера (ВАЖНО!)

```c
// В ARGB_Auto.h или вручную в скетче

// 32-битные таймеры: TIM2, TIM5 на большинстве STM32
cfg->is_32bit_tim = (cfg->tim == TIM2 || cfg->tim == TIM5);

// DMA Data Alignment должен соответствовать!
if (cfg->is_32bit_tim) {
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;      // 32-bit
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
} else {
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;  // 16-bit
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
}
```

**Если забыть про это - CCR не будет обновляться корректно!**

#### 2. Режим DMA

```c
hdma.Init.Mode = DMA_NORMAL;  // НЕ CIRCULAR!
```

Библиотека использует NORMAL режим с полной передачей буфера.

#### 3. Приоритет

```c
hdma.Init.Priority = DMA_PRIORITY_HIGH;  // Рекомендуется HIGH для PWM
```

### Ручная настройка (без ARGB_Auto.h)

Если автоанализ не поддерживает ваш чип, настройте вручную:

```cpp
// 1. Включите тактирование
__HAL_RCC_TIM3_CLK_ENABLE();
__HAL_RCC_DMA1_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();

// 2. GPIO (найдите AF в даташите!)
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_4;            // PB4
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;   // TIM3_CH1 на PB4
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// 3. Timer
htim3.Instance = TIM3;
htim3.Init.Prescaler = 0;
htim3.Init.Period = 100 - 1;  // ARGB_Init() пересчитает
HAL_TIM_PWM_Init(&htim3);

TIM_OC_InitTypeDef sConfigOC = {0};
sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 0;
HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);

// 4. DMA (найдите Stream/Channel в RM!)
hdma.Instance = DMA1_Stream4;           // TIM3_CH1 -> Stream4
hdma.Init.Channel = DMA_CHANNEL_5;      // Channel 5
hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
hdma.Init.PeriphInc = DMA_PINC_DISABLE;
hdma.Init.MemInc = DMA_MINC_ENABLE;
hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;  // TIM3 - 16-bit
hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
hdma.Init.Mode = DMA_NORMAL;
hdma.Init.Priority = DMA_PRIORITY_HIGH;
HAL_DMA_Init(&hdma);

__HAL_LINKDMA(&htim3, hdma[TIM_DMA_ID_CC1], hdma);

// 5. IRQ
HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 1, 0);
HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

// 6. Передаем в библиотеку
ARGB_Attach(&htim3, TIM_CHANNEL_1, &hdma, timer_clock_hz);
ARGB_Init();
```

---

## Чеклист при добавлении нового чипа

### LED-чип (WS28xx, SK68xx, etc.)

- [ ] Найти тайминги T0H, T1H, T0L, T1L в даташите
- [ ] Найти время RESET (обычно 50-300 мкс)
- [ ] Добавить `#elif defined(MY_LED)` в `ARGB.h`
- [ ] Указать тайминги в наносекундах
- [ ] Если RGBW - добавить `#define ARGB_USE_RGBW`
- [ ] Если не GRB порядок - добавить обработку в `ARGB.c`
- [ ] Протестировать!

### STM32 семейство

- [ ] Определить тип DMA (Stream/Channel, с/без DMAMUX)
- [ ] Найти таблицу DMA Request Mapping в Reference Manual
- [ ] Создать структуру маппинга в `ARGB_Auto.h`
- [ ] Заполнить таблицу для всех нужных таймеров
- [ ] Обновить `ARGB_AnalyzePin()` для нового семейства
- [ ] Определить 32-битные таймеры (обычно TIM2, TIM5)
- [ ] Проверить Alternate Functions для GPIO
- [ ] Протестировать!

### Новый таймер на существующем семействе

- [ ] Найти DMA Stream/Channel в таблице маппинга
- [ ] Добавить строки в `ARGB_DMA_MAP[]`
- [ ] Проверить разрядность (16/32 бит)
- [ ] Добавить `#if defined(TIMx)` guards
- [ ] Протестировать!

---

## Отладка

### Включение отладочного вывода

```c
// В скетче до #include <ARGB.h>
#define ARGB_DEBUG
```

### Проверка буфера PWM

```cpp
extern uint32_t PWM_BUF[];
extern volatile uint8_t PWM_HI;
extern volatile uint8_t PWM_LO;

Serial.print("PWM_HI="); Serial.print(PWM_HI);
Serial.print(" PWM_LO="); Serial.println(PWM_LO);

// Первые 24 значения = первый пиксель (24 бита для RGB)
for (int i = 0; i < 24; i++) {
    Serial.print(PWM_BUF[i]); Serial.print(" ");
}
```

### Типичные проблемы

| Симптом | Возможная причина | Решение |
|---------|-------------------|---------|
| Нет сигнала | Неверный AF GPIO | Проверить Alternate Function |
| Плато HIGH | 32-bit таймер с 16-bit DMA | Использовать `DMA_PDATAALIGN_WORD` |
| Мерцание | DMA в CIRCULAR режиме | Использовать `DMA_NORMAL` |
| Только первый пиксель | Маленький буфер | Проверить `NUM_PIXELS` |
| Все пиксели белые | Яркость 255 + все биты HIGH | Проверить `PWM_LO` != 0 |

### Осциллограф

Правильный сигнал WS2812:
- Период бита: ~1.25 мкс (800 кГц)
- T0H: ~300-400 нс
- T1H: ~600-800 нс
- RESET: >50 мкс LOW

---

## Примеры

### Минимальный пример для STM32F401 (TIM2_CH1 на PA0)

```cpp
#define NUM_PIXELS 8
#define WS2812
#include <ARGB.h>

TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma;

void setup() {
    // ... инициализация TIM2, DMA, GPIO ...
    
    ARGB_Attach(&htim2, TIM_CHANNEL_1, &hdma, 84000000);
    ARGB_Init();
    
    ARGB_SetBrightness(50);
    ARGB_FillRGB(255, 0, 0);  // Все красные
    ARGB_Show();
}

void loop() {
    // Анимация...
    ARGB_Show();
    delay(30);
}
```

### Пример с автоанализом пина

```cpp
#define NUM_PIXELS 8
#define WS2812
#include <ARGB.h>
#include <ARGB_Auto.h>

ARGB_PinConfig_t cfg;

void setup() {
    Serial.begin(115200);
    
    // Автоматически определить конфигурацию для PA0
    if (ARGB_AnalyzePin(PA0, &cfg) == ARGB_DMA_OK) {
        ARGB_PrintPinConfig(PA0);
        // ... настроить TIM и DMA по cfg ...
    } else {
        Serial.println("Pin not supported!");
    }
}
```

---

## Контакты

При проблемах создавайте Issue в репозитории с:
1. Модель STM32
2. Используемый пин и таймер
3. Тип светодиодов
4. Вывод Serial с отладкой
5. Скриншот осциллограммы (если есть)

