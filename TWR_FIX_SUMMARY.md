# TWR Проблемы и решение

## Текущее состояние
- ✅ TAG передает пакеты (`✓ TX OK`)
- ❌ ANCHOR НЕ видит пакеты (Status=0x02800002 постоянный)
- ❌ TX BUFFER ERROR появляются периодически

## Основные проблемы

### 1. Конфигурация модуля НЕПОЛНАЯ
Мы добавили:
```cpp
chanCtrl = 0x001C0000 | 5;  // Канал 5
PANADR = 0xDECA0000;        // PAN ID
```

НО НЕ настроили:
- PRF (Pulse Repetition Frequency) 
- Data Rate
- Preamble Length
- PAC Size
- TX/RX antenna delays

### 2. В официальных примерах используется dwt_configure()
Который настраивает:
```c
dwt_config_t config = {
    2,               /* Channel number */
    DWT_PRF_64M,     /* Pulse repetition frequency */
    DWT_PLEN_128,    /* Preamble length */
    DWT_PAC8,        /* Preamble acquisition chunk size */
    9,               /* TX preamble code */
    9,               /* RX preamble code */
    0,               /* Standard SFD */
    DWT_BR_6M8,      /* Data rate 6.8 Mbps */
    DWT_PHRMODE_STD, /* PHY header mode */
    (129 + 8 - 8)    /* SFD timeout */
};
```

### 3. Antenna Delays НЕ установлены
```c
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436
dwt_setrxantennadelay(RX_ANT_DLY);
dwt_settxantennadelay(TX_ANT_DLY);
```

## Что нужно сделать

### Вариант 1: Использовать ГОТОВУЮ библиотеку
- thotro/arduino-dw1000
- Или официальный Decawave driver портированный на Arduino

### Вариант 2: Реализовать ПОЛНУЮ конфигурацию
Нужно портировать `dwt_configure()` из официального API и настроить ВСЕ регистры.

### Вариант 3: МИНИМАЛЬНЫЙ набор регистров (БЫСТРОЕ РЕШЕНИЕ)
Скопировать конфигурацию из рабочих примеров:

```cpp
// После инициализации в begin():

// 1. Channel configuration
writeRegister32(REG_CHAN_CTRL, 0, 0x001C0002); // Ch 2, PRF 64

// 2. TX Frame Control (уже есть в sendData)

// 3. TX Power
writeRegister32(REG_TX_POWER, 0, 0x0E082848);

// 4. Antenna delays
writeRegister16(REG_TX_ANTD, 0, 16436);
writeRegister16(REG_LDE_CTRL + 0x0806, 0, 16436); // RX ant delay

// 5. AGC_TUNE
writeRegister32(REG_AGC_CTRL + 0x04, 0, 0x8870);

// 6. DRX_TUNE (много регистров)
...
```

## Текущая проблема с TX BUFFER ERROR

Статус `0xFFFFFF7F` или `0xFFFFFF3F` означает что:
- Все биты установлены = 0xFFFFFFFF
- Минус бит TXFRS (0x80) = 0xFFFFFF7F

Это значит **SPI читает 0xFF** что может быть:
1. Статус не очищен перед TX
2. Слишком быстро читаем после TX
3. Модуль завис

Решение:
```cpp
// Перед каждой передачей
writeRegister32(REG_SYS_STATUS, 0, 0xFFFFFFFF); // Clear ALL
delay(1); // Дать время модулю
```

## Рекомендация

**ИСПОЛЬЗУЙТЕ ГОТОВУЮ БИБЛИОТЕКУ!**

Реализация полного драйвера DWM1000 - это сотни строк кода с точной настройкой десятков регистров. Официальный драйвер от Decawave - это 5000+ строк кода.

Для быстрого результата используйте:
https://github.com/thotro/arduino-dw1000

Или портируйте полный dwt_configure() из официальных примеров.
