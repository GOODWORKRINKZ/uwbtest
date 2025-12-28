/*! ----------------------------------------------------------------------------
 * @file    deca_spi_arduino.cpp
 * @brief   SPI access functions for Arduino ESP32-C3
 *
 * Adapted from Decawave deca_spi.c for Arduino framework
 */

#include <Arduino.h>
#include <SPI.h>
#include "deca_spi.h"
#include "deca_device_api.h"
#include "port_arduino.h"

// SPI pins for ESP32-C3
#define DW_CS_PIN   7    // CS pin
#define DW_CLK_PIN  4    // SCK pin  
#define DW_MOSI_PIN 6    // MOSI pin
#define DW_MISO_PIN 5    // MISO pin

SPIClass * dwSPI = nullptr;
static uint32_t currentSpiSpeed = 20000000; // Default 20MHz

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
extern "C" int openspi(void)
{
    if (dwSPI == nullptr) {
        dwSPI = new SPIClass(FSPI);
    }
    
    pinMode(DW_CS_PIN, OUTPUT);
    digitalWrite(DW_CS_PIN, HIGH);
    
    dwSPI->begin(DW_CLK_PIN, DW_MISO_PIN, DW_MOSI_PIN, DW_CS_PIN);
    
    return 0;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
extern "C" int closespi(void)
{
    if (dwSPI != nullptr) {
        dwSPI->end();
    }
    return 0;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success
 */
extern "C" int writetospi(uint16_t headerLength,
                          const uint8_t *headerBuffer,
                          uint32_t bodyLength,
                          const uint8_t *bodyBuffer)
{
    if (dwSPI == nullptr) return -1;
    
    decaIrqStatus_t stat;
    stat = decamutexon();
    
    digitalWrite(DW_CS_PIN, LOW);
    
    dwSPI->beginTransaction(SPISettings(currentSpiSpeed, MSBFIRST, SPI_MODE0));
    
    // Send header
    for (uint16_t i = 0; i < headerLength; i++) {
        dwSPI->transfer(headerBuffer[i]);
    }
    
    // Send body
    for (uint32_t i = 0; i < bodyLength; i++) {
        dwSPI->transfer(bodyBuffer[i]);
    }
    
    dwSPI->endTransaction();
    
    digitalWrite(DW_CS_PIN, HIGH);
    
    decamutexoff(stat);
    
    return 0;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: set_spi_speed()
 *
 * Set SPI clock speed
 */
extern "C" void set_spi_speed(uint32_t speed_hz)
{
    currentSpiSpeed = speed_hz;
}
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns -1 if there was an error
 */
extern "C" int readfromspi(uint16_t headerLength,
                           const uint8_t *headerBuffer,
                           uint32_t readLength,
                           uint8_t *readBuffer)
{
    if (dwSPI == nullptr) return -1;
    
    decaIrqStatus_t stat;
    stat = decamutexon();
    
    digitalWrite(DW_CS_PIN, LOW);
    
    dwSPI->beginTransaction(SPISettings(currentSpiSpeed, MSBFIRST, SPI_MODE0));
    
    // Send header
    for (uint16_t i = 0; i < headerLength; i++) {
        dwSPI->transfer(headerBuffer[i]);
    }
    
    // Read data
    for (uint32_t i = 0; i < readLength; i++) {
        readBuffer[i] = dwSPI->transfer(0x00);
    }
    
    dwSPI->endTransaction();
    
    digitalWrite(DW_CS_PIN, HIGH);
    
    decamutexoff(stat);
    
    return 0;
}
