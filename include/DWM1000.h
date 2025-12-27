/**
 * @file DWM1000.h
 * @brief Библиотека для работы с модулем DWM1000 UWB
 * 
 * Этот файл содержит определения регистров и функции для работы
 * с модулем DWM1000 от Qorvo/Decawave
 */

#ifndef DWM1000_H
#define DWM1000_H

#include <Arduino.h>
#include <SPI.h>

// Пины подключения DWM1000 к ESP32-C3
// Настройте эти пины согласно вашей схеме подключения
#define DWM1000_CS_PIN    7    // Chip Select (SS)
#define DWM1000_RST_PIN   10   // Reset
#define DWM1000_IRQ_PIN   2    // Interrupt
#define DWM1000_MOSI_PIN  6    // MOSI
#define DWM1000_MISO_PIN  5    // MISO
#define DWM1000_SCK_PIN   4    // SCK

// Основные регистры DWM1000
#define REG_DEV_ID        0x00  // Device ID
#define REG_EUI           0x01  // Extended Unique Identifier
#define REG_PANADR        0x03  // PAN Identifier and Short Address
#define REG_SYS_CFG       0x04  // System Configuration
#define REG_SYS_TIME      0x06  // System Time Counter
#define REG_TX_FCTRL      0x08  // Transmit Frame Control
#define REG_TX_BUFFER     0x09  // Transmit Data Buffer
#define REG_DX_TIME       0x0A  // Delayed Send or Receive Time
#define REG_RX_FWTO       0x0C  // Receive Frame Wait Timeout Period
#define REG_SYS_CTRL      0x0D  // System Control Register
#define REG_SYS_MASK      0x0E  // System Event Mask Register
#define REG_SYS_STATUS    0x0F  // System Event Status Register
#define REG_RX_FINFO      0x10  // RX Frame Information
#define REG_RX_BUFFER     0x11  // Receive Data Buffer
#define REG_RX_FQUAL      0x12  // Rx Frame Quality information
#define REG_RX_TTCKI      0x13  // Receiver Time Tracking Interval
#define REG_RX_TTCKO      0x14  // Receiver Time Tracking Offset
#define REG_RX_TIME       0x15  // Receive Message Time of Arrival
#define REG_TX_TIME       0x17  // Transmit Message Time of Sending
#define REG_TX_ANTD       0x18  // Transmitter Antenna Delay
#define REG_SYS_STATE     0x19  // System State information
#define REG_ACK_RESP_T    0x1A  // Acknowledgement Time and Response Time
#define REG_RX_SNIFF      0x1D  // Sniff Mode Configuration
#define REG_TX_POWER      0x1E  // TX Power Control
#define REG_CHAN_CTRL     0x1F  // Channel Control
#define REG_USR_SFD       0x21  // User-specified short/long TX/RX SFD sequences
#define REG_AGC_CTRL      0x23  // Automatic Gain Control configuration
#define REG_EXT_SYNC      0x24  // External synchronisation control
#define REG_ACC_MEM       0x25  // Read access to accumulator data
#define REG_GPIO_CTRL     0x26  // Peripheral register bus 1 access - GPIO control
#define REG_DRX_CONF      0x27  // Digital Receiver configuration
#define REG_RF_CONF       0x28  // Analog RF Configuration
#define REG_TX_CAL        0x2A  // Transmitter calibration block
#define REG_FS_CTRL       0x2B  // Frequency synthesiser control block
#define REG_AON           0x2C  // Always-On register set
#define REG_OTP_IF        0x2D  // One Time Programmable Memory Interface
#define REG_LDE_CTRL      0x2E  // Leading edge detection control block
#define REG_DIG_DIAG      0x2F  // Digital Diagnostics Interface
#define REG_PMSC          0x36  // Power Management System Control Block

// Команды управления системой
#define SYS_CTRL_TXSTRT   0x00000002  // Начать передачу
#define SYS_CTRL_RXENAB   0x00000100  // Включить приемник
#define SYS_CTRL_TRXOFF   0x00000040  // Выключить приемопередатчик

// Биты статуса системы
#define SYS_STATUS_TXFRS  0x00000080  // Передача завершена
#define SYS_STATUS_RXFCG  0x00002000  // Кадр принят корректно
#define SYS_STATUS_RXFCE  0x00008000  // Ошибка CRC

// Ожидаемый Device ID для DWM1000
#define DWM1000_DEVICE_ID 0xDECA0130

class DWM1000 {
public:
    DWM1000();
    
    // Основные функции
    bool begin();
    void reset();
    uint32_t getDeviceID();
    void getEUI(uint8_t* eui);
    
    // Функции для тестирования
    void printDeviceInfo();
    void printRegisters();
    bool testCommunication();
    
    // Конфигурация
    void setChannel(uint8_t channel);
    void setPRF(uint8_t prf);
    void setDataRate(uint8_t rate);
    
    // Режимы работы
    void enableTransmitter();
    void enableReceiver();
    void disableTransceiver();
    
    // Передача и прием
    void sendData(uint8_t* data, uint16_t len);
    uint16_t receiveData(uint8_t* buffer, uint16_t maxLen);
    
    // Режим измерения расстояния
    void initRanging();
    float getDistance();
    
private:
    SPIClass* spi;
    bool initialized;
    
    // Внутренние функции для работы с SPI
    void writeRegister(uint8_t reg, uint8_t* data, uint16_t len);
    void readRegister(uint8_t reg, uint8_t* data, uint16_t len);
    void writeRegister8(uint8_t reg, uint16_t offset, uint8_t value);
    void writeRegister16(uint8_t reg, uint16_t offset, uint16_t value);
    void writeRegister32(uint8_t reg, uint16_t offset, uint32_t value);
    uint8_t readRegister8(uint8_t reg, uint16_t offset);
    uint16_t readRegister16(uint8_t reg, uint16_t offset);
    uint32_t readRegister32(uint8_t reg, uint16_t offset);
    
    void selectChip();
    void deselectChip();
};

#endif // DWM1000_H
