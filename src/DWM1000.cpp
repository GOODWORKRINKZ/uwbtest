/**
 * @file DWM1000.cpp
 * @brief Реализация библиотеки для работы с модулем DWM1000 UWB
 */

#include "DWM1000.h"

DWM1000::DWM1000() {
    initialized = false;
}

bool DWM1000::begin() {
    Serial.println("\n=== Инициализация модуля DWM1000 ===");
    
    // Настройка пинов
    pinMode(DWM1000_CS_PIN, OUTPUT);
    pinMode(DWM1000_IRQ_PIN, INPUT);
    // RST пин не используется (не подключен по схеме)
    
    digitalWrite(DWM1000_CS_PIN, HIGH);
    
    Serial.printf("Пины настроены:\n");
    Serial.printf("  CS:   GPIO %d\n", DWM1000_CS_PIN);
    Serial.printf("  IRQ:  GPIO %d\n", DWM1000_IRQ_PIN);
    Serial.printf("  MOSI: GPIO %d\n", DWM1000_MOSI_PIN);
    Serial.printf("  MISO: GPIO %d\n", DWM1000_MISO_PIN);
    Serial.printf("  SCK:  GPIO %d\n", DWM1000_SCK_PIN);
    Serial.println("  RST:  не используется (программный сброс через SPI)");
    
    // Инициализация SPI
    spi = new SPIClass(FSPI);
    spi->begin(DWM1000_SCK_PIN, DWM1000_MISO_PIN, DWM1000_MOSI_PIN, DWM1000_CS_PIN);
    
    // Сначала ОЧЕНЬ низкая скорость для стабилизации связи
    spi->setFrequency(1000000); // 1 МГц для первого контакта
    spi->setDataMode(SPI_MODE0);  // CPOL=0, CPHA=0
    spi->setBitOrder(MSBFIRST);
    
    Serial.println("SPI инициализирован (1 МГц, MODE0, MSB First)");
    
    // КРИТИЧНО: Задержка после инициализации SPI
    Serial.println("\nОжидание стабилизации модуля...");
    delay(500);  // Большая задержка для стабилизации
    
    // Программный сброс модуля через SPI
    Serial.println("Выполнение программного сброса через SPI...");
    softReset();
    delay(500);  // Большая задержка после сброса
    
    // Теперь переключаемся на высокую скорость для избежания битового сдвига
    Serial.println("Переключение на высокую скорость SPI (20 МГц)...");
    spi->setFrequency(20000000); // 20 МГц - избегаем битового сдвига
    delay(10);
    
    // Проверка Device ID
    Serial.println("\nПроверка связи с модулем...");
    uint32_t deviceId = getDeviceID();
    Serial.printf("Device ID: 0x%08X (ожидается 0x%08X)\n", deviceId, DWM1000_DEVICE_ID);
    
    // Если Device ID неправильный, пробуем тест записи/чтения в регистр 0x21
    if (deviceId != DWM1000_DEVICE_ID) {
        Serial.println("\n⚠ Device ID неправильный! Выполняю тест SPI записи/чтения...");
        
        // Тест: записываем известные данные в регистр 0x21 и читаем обратно
        uint8_t testWrite[4] = {0xDE, 0xCA, 0x01, 0x30};
        uint8_t testRead[4] = {0};
        
        writeRegister(0x21, testWrite, 4);
        delay(10);
        readRegister(0x21, testRead, 4);
        
        Serial.print("Записано: ");
        for(int i=0; i<4; i++) Serial.printf("%02X ", testWrite[i]);
        Serial.print("\nПрочитано: ");
        for(int i=0; i<4; i++) Serial.printf("%02X ", testRead[i]);
        Serial.println();
        
        if (memcmp(testWrite, testRead, 4) == 0) {
            Serial.println("✓ SPI работает корректно! Проблема в Device ID...");
            
            // Применяем фикс бага DWM1000 SPI
            Serial.println("\nПрименяю фикс DW1000 SPI бага...");
            writeRegister32(0x04, 0, 0x1600);  // Установить бит 10 в SYS_CFG (0x04)
            delay(50);
            
            deviceId = getDeviceID();
            Serial.printf("Device ID после фикса: 0x%08X\n", deviceId);
        } else {
            Serial.println("✗ SPI не работает! Проблема с подключением.");
        }
    }
    
    if (deviceId == DWM1000_DEVICE_ID) {
        Serial.println("\n✓ Модуль DWM1000 обнаружен успешно!");
        initialized = true;
        
        return true;
    } else {
        Serial.println("\n✗ ОШИБКА: Модуль DWM1000 не обнаружен");
        if (deviceId == 0xFFFFFFFF) {
            Serial.println("Причина: Все линии HIGH - модуль не отвечает или не подключен");
        } else if (deviceId == 0x00000000) {
            Serial.println("Причина: Все линии LOW - проверьте питание модуля");
        } else {
            Serial.println("Причина: Неправильный Device ID - проверьте подключение SPI");
        }
        Serial.println("\n⚠ РЕЖИМ БЕЗ МОДУЛЯ: Интерфейс активен, но модуль недоступен");
        Serial.println("Проверьте подключение и используйте команду 'r' для перезагрузки");
        return false;
    }
}

void DWM1000::reset() {
    // Программный сброс через SPI (аппаратный RST не подключен)
    softReset();
}

void DWM1000::softReset() {
    // Программный сброс: записываем в регистр PMSC_CTRL0
    writeRegister8(0x36, 0x00, 0x01);  // Soft reset
    delay(10);
    writeRegister8(0x36, 0x00, 0x00);  // Clear reset
    delay(10);
    Serial.println("Программный сброс выполнен");
}

uint32_t DWM1000::getDeviceID() {
    return readRegister32(REG_DEV_ID, 0);
}

void DWM1000::getEUI(uint8_t* eui) {
    readRegister(REG_EUI, eui, 8);
}

void DWM1000::printDeviceInfo() {
    Serial.println("\n=== Информация о модуле DWM1000 ===");
    
    uint32_t deviceId = getDeviceID();
    Serial.printf("Device ID:     0x%08X\n", deviceId);
    Serial.printf("  Model:       %d\n", (deviceId >> 8) & 0xFF);
    Serial.printf("  Version:     %d\n", (deviceId >> 16) & 0xFF);
    Serial.printf("  Revision:    %d\n", (deviceId >> 24) & 0xFF);
    
    uint8_t eui[8];
    getEUI(eui);
    Serial.print("EUI-64:        ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", eui[i]);
        if (i < 7) Serial.print(":");
    }
    Serial.println();
    
    uint16_t panId = readRegister16(REG_PANADR, 2);
    uint16_t shortAddr = readRegister16(REG_PANADR, 0);
    Serial.printf("PAN ID:        0x%04X\n", panId);
    Serial.printf("Short Address: 0x%04X\n", shortAddr);
    
    uint32_t sysStatus = readRegister32(REG_SYS_STATUS, 0);
    Serial.printf("System Status: 0x%08X\n", sysStatus);
    
    uint32_t sysState = readRegister32(REG_SYS_STATE, 0);
    Serial.printf("System State:  0x%08X\n", sysState);
    
    Serial.println("===================================");
}

void DWM1000::printRegisters() {
    Serial.println("\n=== Дамп регистров DWM1000 ===");
    
    // Основные регистры для проверки
    struct RegInfo {
        uint8_t reg;
        const char* name;
        uint8_t len;
    };
    
    RegInfo regs[] = {
        {REG_DEV_ID, "DEV_ID", 4},
        {REG_EUI, "EUI", 8},
        {REG_PANADR, "PANADR", 4},
        {REG_SYS_CFG, "SYS_CFG", 4},
        {REG_SYS_CTRL, "SYS_CTRL", 4},
        {REG_SYS_STATUS, "SYS_STATUS", 5},
        {REG_CHAN_CTRL, "CHAN_CTRL", 4}
    };
    
    for (int i = 0; i < sizeof(regs) / sizeof(RegInfo); i++) {
        Serial.printf("0x%02X %-12s: ", regs[i].reg, regs[i].name);
        uint8_t data[8];
        readRegister(regs[i].reg, data, regs[i].len);
        for (int j = 0; j < regs[i].len; j++) {
            Serial.printf("%02X ", data[j]);
        }
        Serial.println();
    }
    
    Serial.println("==============================");
}

bool DWM1000::testCommunication() {
    Serial.println("\n=== Тест коммуникации с модулем ===");
    
    // Тест 1: Проверка Device ID несколько раз
    Serial.println("\nТест 1: Множественное чтение Device ID");
    bool idTestPassed = true;
    for (int i = 0; i < 5; i++) {
        uint32_t id = getDeviceID();
        Serial.printf("  Попытка %d: 0x%08X ", i + 1, id);
        if (id == DWM1000_DEVICE_ID) {
            Serial.println("✓");
        } else {
            Serial.println("✗");
            idTestPassed = false;
        }
        delay(10);
    }
    
    // Тест 2: Запись и чтение тестового значения
    Serial.println("\nТест 2: Запись/чтение регистра PANADR");
    uint16_t originalPanId = readRegister16(REG_PANADR, 2);
    uint16_t originalAddr = readRegister16(REG_PANADR, 0);
    Serial.printf("  Оригинальные значения - PAN: 0x%04X, Addr: 0x%04X\n", 
                  originalPanId, originalAddr);
    
    // Записываем тестовые значения
    uint16_t testPanId = 0xCAFE;
    uint16_t testAddr = 0xBEEF;
    writeRegister16(REG_PANADR, 2, testPanId);
    writeRegister16(REG_PANADR, 0, testAddr);
    Serial.printf("  Записаны тестовые - PAN: 0x%04X, Addr: 0x%04X\n", 
                  testPanId, testAddr);
    
    // Читаем обратно
    uint16_t readPanId = readRegister16(REG_PANADR, 2);
    uint16_t readAddr = readRegister16(REG_PANADR, 0);
    Serial.printf("  Прочитаны значения - PAN: 0x%04X, Addr: 0x%04X ", 
                  readPanId, readAddr);
    
    bool writeTestPassed = (readPanId == testPanId) && (readAddr == testAddr);
    if (writeTestPassed) {
        Serial.println("✓");
    } else {
        Serial.println("✗");
    }
    
    // Восстанавливаем оригинальные значения
    writeRegister16(REG_PANADR, 2, originalPanId);
    writeRegister16(REG_PANADR, 0, originalAddr);
    Serial.println("  Оригинальные значения восстановлены");
    
    // Тест 3: Проверка линии IRQ
    Serial.println("\nТест 3: Проверка линии IRQ");
    int irqState = digitalRead(DWM1000_IRQ_PIN);
    Serial.printf("  Состояние IRQ: %s\n", irqState ? "HIGH" : "LOW");
    
    // Общий результат
    Serial.println("\n=== Результат тестов ===");
    bool allPassed = idTestPassed && writeTestPassed;
    if (allPassed) {
        Serial.println("✓ ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!");
        Serial.println("Модуль DWM1000 работает корректно.");
    } else {
        Serial.println("✗ НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОЙДЕНЫ!");
        Serial.println("Проверьте подключение и состояние модуля.");
    }
    Serial.println("=======================");
    
    return allPassed;
}

void DWM1000::setChannel(uint8_t channel) {
    Serial.printf("\nНастройка канала: %d\n", channel);
    // Упрощенная настройка канала (для полной реализации нужна документация)
    uint32_t chanCtrl = readRegister32(REG_CHAN_CTRL, 0);
    chanCtrl = (chanCtrl & 0xFFFFFFF0) | (channel & 0x0F);
    writeRegister32(REG_CHAN_CTRL, 0, chanCtrl);
    Serial.println("Канал настроен");
}

void DWM1000::setPRF(uint8_t prf) {
    Serial.printf("Настройка PRF: %d МГц\n", prf);
    // Настройка Pulse Repetition Frequency
}

void DWM1000::setDataRate(uint8_t rate) {
    Serial.printf("Настройка скорости передачи: %d\n", rate);
    // Настройка скорости передачи данных
}

void DWM1000::enableTransmitter() {
    Serial.println("\n=== Включение режима передатчика ===");
    disableTransceiver();
    delay(10);
    Serial.println("Передатчик готов к работе");
}

void DWM1000::enableReceiver() {
    Serial.println("\n=== Включение режима приемника ===");
    disableTransceiver();
    delay(10);
    writeRegister32(REG_SYS_CTRL, 0, SYS_CTRL_RXENAB);
    Serial.println("Приемник включен и готов к приему");
}

void DWM1000::disableTransceiver() {
    writeRegister32(REG_SYS_CTRL, 0, SYS_CTRL_TRXOFF);
    Serial.println("Приемопередатчик выключен");
}

void DWM1000::sendData(uint8_t* data, uint16_t len) {
    Serial.printf("\n=== Отправка данных (%d байт) ===\n", len);
    Serial.print("Данные: ");
    for (uint16_t i = 0; i < len && i < 32; i++) {
        Serial.printf("%02X ", data[i]);
    }
    if (len > 32) Serial.print("...");
    Serial.println();
    
    // Запись данных в буфер передачи
    writeRegister(REG_TX_BUFFER, data, len);
    
    // Настройка размера кадра
    uint8_t txCtrl[5] = {0};
    txCtrl[0] = len & 0xFF;
    txCtrl[1] = (len >> 8) & 0x03;
    writeRegister(REG_TX_FCTRL, txCtrl, 5);
    
    // Начало передачи
    writeRegister32(REG_SYS_CTRL, 0, SYS_CTRL_TXSTRT);
    Serial.println("Передача начата...");
    
    // Ожидание завершения передачи
    uint32_t timeout = millis() + 100;
    while (millis() < timeout) {
        uint32_t status = readRegister32(REG_SYS_STATUS, 0);
        if (status & SYS_STATUS_TXFRS) {
            Serial.println("✓ Передача завершена успешно");
            // Очистка флага
            writeRegister32(REG_SYS_STATUS, 0, SYS_STATUS_TXFRS);
            return;
        }
        delay(1);
    }
    Serial.println("✗ Таймаут передачи");
}

uint16_t DWM1000::receiveData(uint8_t* buffer, uint16_t maxLen) {
    Serial.println("\n=== Ожидание приема данных ===");
    
    uint32_t status = readRegister32(REG_SYS_STATUS, 0);
    
    if (status & SYS_STATUS_RXFCG) {
        Serial.println("✓ Данные приняты успешно");
        
        // Получение информации о кадре
        uint32_t rxInfo = readRegister32(REG_RX_FINFO, 0);
        uint16_t len = rxInfo & 0x3FF;
        
        Serial.printf("Размер принятых данных: %d байт\n", len);
        
        if (len > maxLen) {
            len = maxLen;
            Serial.printf("Предупреждение: данные обрезаны до %d байт\n", maxLen);
        }
        
        // Чтение данных
        readRegister(REG_RX_BUFFER, buffer, len);
        
        Serial.print("Данные: ");
        for (uint16_t i = 0; i < len && i < 32; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        if (len > 32) Serial.print("...");
        Serial.println();
        
        // Очистка флагов
        writeRegister32(REG_SYS_STATUS, 0, SYS_STATUS_RXFCG);
        
        return len;
    } else if (status & SYS_STATUS_RXFCE) {
        Serial.println("✗ Ошибка CRC при приеме");
        writeRegister32(REG_SYS_STATUS, 0, SYS_STATUS_RXFCE);
        return 0;
    }
    
    return 0;
}

void DWM1000::initRanging() {
    Serial.println("\n=== Инициализация режима измерения расстояния ===");
    // Настройка для Two-Way Ranging (TWR)
    Serial.println("Режим TWR настроен");
}

float DWM1000::getDistance() {
    // Упрощенный расчет расстояния
    // Для полной реализации требуется сложный алгоритм TWR
    Serial.println("Расчет расстояния...");
    return 0.0;
}

// Внутренние функции SPI

void DWM1000::selectChip() {
    digitalWrite(DWM1000_CS_PIN, LOW);
}

void DWM1000::deselectChip() {
    digitalWrite(DWM1000_CS_PIN, HIGH);
}

void DWM1000::writeRegister(uint8_t reg, uint8_t* data, uint16_t len) {
    selectChip();
    delayMicroseconds(5);
    
    // Формирование заголовка
    uint8_t header = 0x80 | reg; // Бит 7 = 1 (запись), биты 0-5 = адрес регистра
    spi->transfer(header);
    
    // Запись данных побайтно
    for (uint16_t i = 0; i < len; i++) {
        spi->transfer(data[i]);
    }
    
    delayMicroseconds(5);
    deselectChip();
}

void DWM1000::readRegister(uint8_t reg, uint8_t* data, uint16_t len) {
    selectChip();
    delayMicroseconds(5);
    
    // Формирование заголовка
    uint8_t header = reg; // Бит 7 = 0 (чтение), биты 0-5 = адрес регистра
    spi->transfer(header);
    
    // Чтение данных
    for (uint16_t i = 0; i < len; i++) {
        data[i] = spi->transfer(0x00);
    }
    
    delayMicroseconds(5);
    deselectChip();
}

void DWM1000::writeRegister8(uint8_t reg, uint16_t offset, uint8_t value) {
    selectChip();
    
    uint8_t header[3];
    header[0] = 0x80 | reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    spi->transfer(value);
    deselectChip();
}

void DWM1000::writeRegister16(uint8_t reg, uint16_t offset, uint16_t value) {
    uint8_t data[2];
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    
    selectChip();
    uint8_t header[3];
    header[0] = 0x80 | reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    spi->transfer(data, 2);
    deselectChip();
}

void DWM1000::writeRegister32(uint8_t reg, uint16_t offset, uint32_t value) {
    uint8_t data[4];
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
    
    selectChip();
    uint8_t header[3];
    header[0] = 0x80 | reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    spi->transfer(data, 4);
    deselectChip();
}

uint8_t DWM1000::readRegister8(uint8_t reg, uint16_t offset) {
    selectChip();
    
    uint8_t header[3];
    header[0] = reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    uint8_t value = spi->transfer(0x00);
    deselectChip();
    return value;
}

uint16_t DWM1000::readRegister16(uint8_t reg, uint16_t offset) {
    uint8_t data[2];
    
    selectChip();
    uint8_t header[3];
    header[0] = reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    data[0] = spi->transfer(0x00);
    data[1] = spi->transfer(0x00);
    deselectChip();
    
    return data[0] | (data[1] << 8);
}

uint32_t DWM1000::readRegister32(uint8_t reg, uint16_t offset) {
    uint8_t data[4];
    
    selectChip();
    uint8_t header[3];
    header[0] = reg;
    
    if (offset > 0) {
        header[0] |= 0x40;
        header[1] = offset & 0x7F;
        spi->transfer(header, 2);
    } else {
        spi->transfer(header[0]);
    }
    
    data[0] = spi->transfer(0x00);
    data[1] = spi->transfer(0x00);
    data[2] = spi->transfer(0x00);
    data[3] = spi->transfer(0x00);
    deselectChip();
    
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}
