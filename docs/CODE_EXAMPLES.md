# Примеры кода

Этот файл содержит примеры кода для расширения функциональности проекта.

## Пример 1: Настройка мощности передачи

```cpp
// В DWM1000.h добавьте:
void setTxPower(int8_t power);

// В DWM1000.cpp реализуйте:
void DWM1000::setTxPower(int8_t power) {
    Serial.printf("Установка мощности передачи: %d dBm\n", power);
    
    // Настройка мощности через регистр TX_POWER
    // Значения зависят от канала и PRF
    // Подробности в документации DW1000 User Manual, раздел 7.2.31
    
    uint32_t txPower = 0;
    
    // Пример для канала 5, PRF 64 MHz
    if (power >= 0) {
        txPower = 0x1F1F1F1F; // Максимальная мощность
    } else if (power >= -10) {
        txPower = 0x0F0F0F0F; // Средняя мощность
    } else {
        txPower = 0x07070707; // Низкая мощность
    }
    
    writeRegister32(REG_TX_POWER, 0, txPower);
    Serial.println("Мощность установлена");
}

// В main.cpp используйте:
dwm.setTxPower(-3); // Установить мощность -3 dBm
```

## Пример 2: Фильтрация принятых пакетов

```cpp
// Добавьте в main.cpp функцию валидации пакета:

bool validatePacket(uint8_t* data, uint16_t len) {
    // Проверка минимального размера
    if (len < 8) {
        Serial.println("Пакет слишком короткий");
        return false;
    }
    
    // Проверка заголовка
    if (data[0] != 0xAA || data[1] != 0x55) {
        Serial.println("Неверный заголовок пакета");
        return false;
    }
    
    // Проверка контрольной суммы (если реализована)
    // uint8_t checksum = calculateChecksum(data, len - 1);
    // if (checksum != data[len - 1]) {
    //     Serial.println("Ошибка контрольной суммы");
    //     return false;
    // }
    
    return true;
}

// Использование в режиме приемника:
uint8_t rxBuffer[128];
uint16_t len = dwm.receiveData(rxBuffer, 128);

if (len > 0 && validatePacket(rxBuffer, len)) {
    // Обработка валидного пакета
    Serial.println("✓ Пакет валиден");
    processPacket(rxBuffer, len);
} else {
    Serial.println("✗ Пакет отклонен");
}
```

## Пример 3: Измерение RSSI (уровня сигнала)

```cpp
// В DWM1000.h добавьте:
float getRSSI();

// В DWM1000.cpp реализуйте:
float DWM1000::getRSSI() {
    // Чтение регистра RX_FQUAL для получения информации о качестве сигнала
    uint16_t cir_pwr = readRegister16(REG_RX_FQUAL, 0);
    uint16_t rxpacc = readRegister16(REG_RX_FINFO, 4) & 0x0FFF;
    
    // Расчет RSSI (упрощенный)
    if (rxpacc == 0) return -999.0;
    
    float rssi = 10.0 * log10((float)cir_pwr / (float)(rxpacc * rxpacc)) - 115.0;
    
    Serial.printf("RSSI: %.1f dBm\n", rssi);
    return rssi;
}

// Использование после приема пакета:
if (len > 0) {
    float rssi = dwm.getRSSI();
    Serial.printf("Уровень сигнала: %.1f dBm\n", rssi);
}
```

## Пример 4: Настройка скорости передачи данных

```cpp
// В DWM1000.cpp реализуйте полноценную настройку:
void DWM1000::setDataRate(uint8_t rate) {
    Serial.printf("Установка скорости передачи: ");
    
    uint32_t sysCfg = readRegister32(REG_SYS_CFG, 0);
    
    switch(rate) {
        case 0: // 110 kbps
            Serial.println("110 kbps");
            sysCfg &= ~0x000000C0;
            break;
        case 1: // 850 kbps
            Serial.println("850 kbps");
            sysCfg = (sysCfg & ~0x000000C0) | 0x00000040;
            break;
        case 2: // 6.8 Mbps
            Serial.println("6.8 Mbps");
            sysCfg = (sysCfg & ~0x000000C0) | 0x00000080;
            break;
        default:
            Serial.println("Неверное значение!");
            return;
    }
    
    writeRegister32(REG_SYS_CFG, 0, sysCfg);
    Serial.println("Скорость установлена");
}

// В main.cpp:
dwm.setDataRate(2); // 6.8 Mbps
```

## Пример 5: Режим энергосбережения

```cpp
// В DWM1000.h добавьте:
void enterSleepMode();
void wakeUp();

// В DWM1000.cpp:
void DWM1000::enterSleepMode() {
    Serial.println("Переход в режим сна...");
    
    // Отключение приемопередатчика
    disableTransceiver();
    
    // Настройка Always-On регистров для режима сна
    // AON_WCFG: Wake-up Configuration
    writeRegister16(REG_AON, 0x00, 0x0000);
    
    // Переход в deep sleep
    writeRegister8(REG_AON, 0x0C, 0x48);
    
    Serial.println("Модуль в режиме сна");
}

void DWM1000::wakeUp() {
    Serial.println("Пробуждение модуля...");
    
    // Импульс на CS для пробуждения
    selectChip();
    delayMicroseconds(500);
    deselectChip();
    delay(5);
    
    // Проверка что модуль проснулся
    uint32_t devId = getDeviceID();
    if (devId == DWM1000_DEVICE_ID) {
        Serial.println("✓ Модуль проснулся");
    } else {
        Serial.println("✗ Ошибка пробуждения");
    }
}

// В main.cpp:
// Отправка пакета
dwm.sendData(data, len);

// Переход в сон на 5 секунд
dwm.enterSleepMode();
delay(5000);

// Пробуждение
dwm.wakeUp();
```

## Пример 6: Автоматическое подтверждение (ACK)

```cpp
// В DWM1000.h:
void enableAutoAck();
void sendWithAck(uint8_t* data, uint16_t len);

// В DWM1000.cpp:
void DWM1000::enableAutoAck() {
    Serial.println("Включение автоматических подтверждений");
    
    uint32_t sysCfg = readRegister32(REG_SYS_CFG, 0);
    sysCfg |= 0x00000010; // Бит AUTOACK
    writeRegister32(REG_SYS_CFG, 0, sysCfg);
    
    Serial.println("Auto-ACK включен");
}

void DWM1000::sendWithAck(uint8_t* data, uint16_t len) {
    Serial.println("Отправка с ожиданием подтверждения...");
    
    // Установка флага ожидания ACK в заголовке кадра
    data[0] |= 0x20; // Frame Control: ACK Request
    
    sendData(data, len);
    
    // Ожидание ACK
    uint32_t timeout = millis() + 100;
    while (millis() < timeout) {
        uint32_t status = readRegister32(REG_SYS_STATUS, 0);
        
        if (status & 0x00002000) { // ACK received
            Serial.println("✓ Подтверждение получено");
            writeRegister32(REG_SYS_STATUS, 0, 0x00002000);
            return;
        }
        delay(1);
    }
    
    Serial.println("✗ Таймаут ожидания подтверждения");
}
```

## Пример 7: Настройка преамбулы

```cpp
void DWM1000::setPreambleLength(uint16_t length) {
    Serial.printf("Установка длины преамбулы: %d символов\n", length);
    
    uint32_t txCtrl = readRegister32(REG_TX_FCTRL, 0);
    
    // Очистка битов длины преамбулы
    txCtrl &= ~0x00030000;
    
    switch(length) {
        case 64:
            txCtrl |= 0x00010000;
            break;
        case 128:
            txCtrl |= 0x00050000;
            break;
        case 256:
            txCtrl |= 0x00090000;
            break;
        case 512:
            txCtrl |= 0x000D0000;
            break;
        case 1024:
            txCtrl |= 0x00020000;
            break;
        case 2048:
            txCtrl |= 0x00060000;
            break;
        case 4096:
            txCtrl |= 0x000A0000;
            break;
        default:
            Serial.println("Неверная длина преамбулы!");
            return;
    }
    
    writeRegister32(REG_TX_FCTRL, 0, txCtrl);
    Serial.println("Преамбула настроена");
}
```

## Пример 8: Статистика передачи

```cpp
// Добавьте в класс DWM1000:
struct Statistics {
    uint32_t txCount;
    uint32_t txSuccess;
    uint32_t txFail;
    uint32_t rxCount;
    uint32_t rxSuccess;
    uint32_t rxCrcError;
    
    void print() {
        Serial.println("\n=== Статистика ===");
        Serial.printf("Передано: %lu (успешно: %lu, ошибок: %lu)\n", 
                     txCount, txSuccess, txFail);
        Serial.printf("Принято: %lu (успешно: %lu, CRC ошибок: %lu)\n",
                     rxCount, rxSuccess, rxCrcError);
        Serial.printf("Успешность TX: %.1f%%\n", 
                     txCount > 0 ? (100.0 * txSuccess / txCount) : 0);
        Serial.printf("Успешность RX: %.1f%%\n",
                     rxCount > 0 ? (100.0 * rxSuccess / rxCount) : 0);
        Serial.println("==================");
    }
    
    void reset() {
        txCount = txSuccess = txFail = 0;
        rxCount = rxSuccess = rxCrcError = 0;
    }
};

Statistics stats;

// Обновляйте статистику в функциях sendData и receiveData
```

## Пример 9: Множественные устройства

```cpp
// Настройка адресов для работы в сети
void DWM1000::setAddress(uint16_t panId, uint16_t shortAddr) {
    Serial.printf("Установка адреса: PAN=0x%04X, Addr=0x%04X\n", 
                  panId, shortAddr);
    
    writeRegister16(REG_PANADR, 2, panId);
    writeRegister16(REG_PANADR, 0, shortAddr);
    
    Serial.println("Адрес установлен");
}

// Отправка пакета конкретному устройству
void DWM1000::sendTo(uint16_t destAddr, uint8_t* data, uint16_t len) {
    // Добавление адреса назначения в заголовок пакета
    uint8_t frame[128];
    frame[0] = 0x41; // Frame Control
    frame[1] = 0x88; // Frame Control
    frame[2] = 0x00; // Sequence Number
    frame[3] = destAddr & 0xFF; // Dest PAN ID (low)
    frame[4] = (destAddr >> 8) & 0xFF; // Dest PAN ID (high)
    frame[5] = destAddr & 0xFF; // Dest Addr (low)
    frame[6] = (destAddr >> 8) & 0xFF; // Dest Addr (high)
    
    // Копирование данных
    memcpy(&frame[7], data, len);
    
    sendData(frame, len + 7);
}

// В main.cpp:
dwm.setAddress(0xCAFE, 0x0001); // Устройство 1
// или
dwm.setAddress(0xCAFE, 0x0002); // Устройство 2

// Отправка устройству 0x0002
uint8_t msg[] = "Hello!";
dwm.sendTo(0x0002, msg, sizeof(msg));
```

## Пример 10: Логирование на SD карту

```cpp
#include <SD.h>
#include <SPI.h>

#define SD_CS_PIN 21

File logFile;

void initSDCard() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("✗ SD карта не найдена");
        return;
    }
    Serial.println("✓ SD карта инициализирована");
    
    logFile = SD.open("/uwb_log.txt", FILE_APPEND);
    if (logFile) {
        logFile.println("\n=== Новая сессия ===");
        logFile.close();
    }
}

void logPacket(const char* direction, uint8_t* data, uint16_t len) {
    logFile = SD.open("/uwb_log.txt", FILE_APPEND);
    if (!logFile) return;
    
    logFile.printf("[%lu] %s: ", millis(), direction);
    for (uint16_t i = 0; i < len && i < 32; i++) {
        logFile.printf("%02X ", data[i]);
    }
    logFile.println();
    logFile.close();
}

// Использование:
// В режиме TX:
dwm.sendData(testData, 32);
logPacket("TX", testData, 32);

// В режиме RX:
len = dwm.receiveData(rxBuffer, 128);
if (len > 0) {
    logPacket("RX", rxBuffer, len);
}
```

---

**Примечание:** Эти примеры показывают возможные расширения функциональности. Для полной реализации обращайтесь к документации DW1000 User Manual.
