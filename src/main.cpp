/**
 * @file main.cpp
 * @brief Тестовая программа для модуля DWM1000 на ESP32-C3
 * 
 * Эта программа демонстрирует различные режимы работы модуля DWM1000:
 * - Инициализация и проверка связи
 * - Режим передатчика
 * - Режим приемника
 * - Режим измерения расстояния
 * 
 * Для управления используется Serial Monitor (115200 baud)
 */

#include <Arduino.h>
#include "DWM1000.h"

DWM1000 dwm;

// Режимы работы
enum TestMode {
    MODE_INIT,          // Начальная инициализация
    MODE_INFO,          // Вывод информации о модуле
    MODE_TEST_COMM,     // Тест коммуникации
    MODE_TRANSMITTER,   // Режим передатчика
    MODE_RECEIVER,      // Режим приемника
    MODE_RANGING,       // Режим измерения расстояния
    MODE_MENU           // Показать меню
};

TestMode currentMode = MODE_INIT;
bool moduleReady = false;

// Объявление функции повтора строки
String repeat(const char* str, int count);

void printMenu() {
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║   МЕНЮ ТЕСТИРОВАНИЯ МОДУЛЯ DWM1000         ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.println("║ 1 - Информация о модуле                    ║");
    Serial.println("║ 2 - Тест коммуникации                      ║");
    Serial.println("║ 3 - Дамп регистров                         ║");
    Serial.println("║ 4 - Режим передатчика (отправка тестовых   ║");
    Serial.println("║     данных каждые 2 секунды)               ║");
    Serial.println("║ 5 - Режим приемника (ожидание данных)      ║");
    Serial.println("║ 6 - Режим измерения расстояния (TWR)       ║");
    Serial.println("║ 7 - Настройка канала                       ║");
    Serial.println("║ 8 - Сброс модуля                           ║");
    Serial.println("║ m - Показать это меню                      ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.println("\nВведите номер режима:");
}

void setup() {
    // Инициализация Serial
    Serial.begin(115200);
    delay(2000);  // Увеличена задержка для инициализации Serial
    
    Serial.println("\n\n");
    Serial.println("================================================");
    Serial.println("  ТЕСТИРОВАНИЕ МОДУЛЯ DWM1000 НА ESP32-C3");
    Serial.println("  Ultra-Wideband (UWB) модуль от Qorvo/Decawave");
    Serial.println("================================================");
    Serial.println();
    Serial.println(">>> Serial Monitor подключен! <<<");
    
    Serial.println("\nИнформация о плате:");
    Serial.printf("  Модель: ESP32-C3 Super Mini\n");
    Serial.printf("  Частота CPU: %d МГц\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash: %d байт\n", ESP.getFlashChipSize());
    Serial.printf("  Free Heap: %d байт\n", ESP.getFreeHeap());
    
    // Инициализация модуля DWM1000
    Serial.println("\n================================================");
    Serial.println("Начинается инициализация модуля DWM1000...");
    Serial.println("================================================\n");
    
    moduleReady = dwm.begin();
    
    Serial.println("\n================================================");
    Serial.println("Инициализация завершена");
    Serial.println("================================================");
    
    if (moduleReady) {
        Serial.println("\n✓ МОДУЛЬ ГОТОВ К РАБОТЕ!\n");
        dwm.printDeviceInfo();
        printMenu();
    } else {
        Serial.println("\n✗ ОШИБКА ИНИЦИАЛИЗАЦИИ!");
        Serial.println("\nПроверьте:");
        Serial.println("  1. Правильность подключения проводов");
        Serial.println("  2. Питание модуля (3.3V)");
        Serial.println("  3. Соответствие пинов в DWM1000.h");
        Serial.println("\nПерезагрузите плату после исправления...");
    }
}

void loop() {
    // Проверка команд из Serial
    if (Serial.available()) {
        char cmd = Serial.read();
        
        // Очистка буфера
        while (Serial.available()) Serial.read();
        
        switch (cmd) {
            case '1':
                Serial.println("\n>>> Выбран режим: Информация о модуле");
                if (moduleReady) {
                    dwm.printDeviceInfo();
                } else {
                    Serial.println("✗ Модуль не готов!");
                }
                break;
                
            case '2':
                Serial.println("\n>>> Выбран режим: Тест коммуникации");
                if (moduleReady) {
                    dwm.testCommunication();
                } else {
                    Serial.println("✗ Модуль не готов!");
                }
                break;
                
            case '3':
                Serial.println("\n>>> Выбран режим: Дамп регистров");
                if (moduleReady) {
                    dwm.printRegisters();
                } else {
                    Serial.println("✗ Модуль не готов!");
                }
                break;
                
            case '4':
                Serial.println("\n>>> Выбран режим: Передатчик");
                currentMode = MODE_TRANSMITTER;
                if (moduleReady) {
                    dwm.enableTransmitter();
                    Serial.println("Отправка данных каждые 2 секунды...");
                    Serial.println("Нажмите любую клавишу для возврата в меню");
                } else {
                    Serial.println("✗ Модуль не готов!");
                    currentMode = MODE_MENU;
                }
                break;
                
            case '5':
                Serial.println("\n>>> Выбран режим: Приемник");
                currentMode = MODE_RECEIVER;
                if (moduleReady) {
                    dwm.enableReceiver();
                    Serial.println("Ожидание данных...");
                    Serial.println("Нажмите любую клавишу для возврата в меню");
                } else {
                    Serial.println("✗ Модуль не готов!");
                    currentMode = MODE_MENU;
                }
                break;
                
            case '6':
                Serial.println("\n>>> Выбран режим: Измерение расстояния");
                currentMode = MODE_RANGING;
                if (moduleReady) {
                    dwm.initRanging();
                    Serial.println("Режим TWR активирован");
                    Serial.println("Нажмите любую клавишу для возврата в меню");
                } else {
                    Serial.println("✗ Модуль не готов!");
                    currentMode = MODE_MENU;
                }
                break;
                
            case '7': {
                Serial.println("\n>>> Настройка канала");
                Serial.println("Доступные каналы: 1, 2, 3, 4, 5, 7");
                Serial.println("Введите номер канала:");
                
                while (!Serial.available()) delay(10);
                uint8_t channel = Serial.parseInt();
                while (Serial.available()) Serial.read();
                
                if (moduleReady && (channel >= 1 && channel <= 7 && channel != 6)) {
                    dwm.setChannel(channel);
                } else {
                    Serial.println("✗ Неверный канал или модуль не готов!");
                }
                break;
            }
                
            case '8':
                Serial.println("\n>>> Сброс модуля");
                if (moduleReady) {
                    dwm.reset();
                    delay(100);
                    moduleReady = dwm.begin();
                } else {
                    Serial.println("✗ Модуль не готов!");
                }
                break;
                
            case 'm':
            case 'M':
                currentMode = MODE_MENU;
                break;
                
            default:
                if (currentMode == MODE_TRANSMITTER || 
                    currentMode == MODE_RECEIVER || 
                    currentMode == MODE_RANGING) {
                    // Выход из активного режима
                    Serial.println("\n>>> Возврат в меню");
                    if (moduleReady) {
                        dwm.disableTransceiver();
                    }
                    currentMode = MODE_MENU;
                } else {
                    Serial.println("Неизвестная команда. Нажмите 'm' для меню.");
                }
                break;
        }
        
        if (currentMode == MODE_MENU) {
            printMenu();
        }
    }
    
    // Обработка активных режимов
    static unsigned long lastAction = 0;
    unsigned long now = millis();
    
    switch (currentMode) {
        case MODE_TRANSMITTER:
            if (now - lastAction >= 2000) {
                lastAction = now;
                
                // Формирование тестовых данных
                uint8_t testData[32];
                testData[0] = 0xAA; // Заголовок
                testData[1] = 0x55;
                testData[2] = (now >> 24) & 0xFF; // Временная метка
                testData[3] = (now >> 16) & 0xFF;
                testData[4] = (now >> 8) & 0xFF;
                testData[5] = now & 0xFF;
                
                // Счетчик пакетов
                static uint16_t packetCount = 0;
                testData[6] = (packetCount >> 8) & 0xFF;
                testData[7] = packetCount & 0xFF;
                packetCount++;
                
                // Заполнение оставшихся данных
                for (int i = 8; i < 32; i++) {
                    testData[i] = i;
                }
                
                Serial.printf("\n[%lu мс] Пакет #%d:\n", now, packetCount - 1);
                dwm.sendData(testData, 32);
            }
            break;
            
        case MODE_RECEIVER:
            if (now - lastAction >= 100) {
                lastAction = now;
                
                uint8_t rxBuffer[128];
                uint16_t len = dwm.receiveData(rxBuffer, 128);
                
                if (len > 0) {
                    Serial.printf("\n[%lu мс] Получен пакет:\n", now);
                    
                    // Расшифровка данных если это наш формат
                    if (len >= 8 && rxBuffer[0] == 0xAA && rxBuffer[1] == 0x55) {
                        uint32_t timestamp = (rxBuffer[2] << 24) | 
                                            (rxBuffer[3] << 16) | 
                                            (rxBuffer[4] << 8) | 
                                            rxBuffer[5];
                        uint16_t packetNum = (rxBuffer[6] << 8) | rxBuffer[7];
                        
                        Serial.printf("  Временная метка: %lu мс\n", timestamp);
                        Serial.printf("  Номер пакета: %d\n", packetNum);
                        Serial.printf("  Задержка: %lu мс\n", now - timestamp);
                    }
                }
            }
            break;
            
        case MODE_RANGING:
            if (now - lastAction >= 500) {
                lastAction = now;
                
                float distance = dwm.getDistance();
                Serial.printf("[%lu мс] Расстояние: %.2f м\n", now, distance);
            }
            break;
            
        default:
            break;
    }
    
    delay(10);
}

// Функция повтора строки
String repeat(const char* str, int count) {
    String result = "";
    for (int i = 0; i < count; i++) {
        result += str;
    }
    return result;
}
