/*! ----------------------------------------------------------------------------
 * @file    port_arduino.cpp
 * @brief   HW specific definitions and functions for Arduino ESP32-C3
 *
 * Adapted from Decawave port.c for Arduino framework
 */

#include <Arduino.h>
#include "port_arduino.h"
#include "deca_device_api.h"
#include "deca_spi.h"

// DW1000 reset pin - physically connected to GPIO2
#define DW_RST_PIN 2

// IRQ/interrupt pin - physically connected to GPIO10
#define DW_IRQ_PIN 10

/* DW1000 IRQ handler */
port_deca_isr_t port_deca_isr = nullptr;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_deca_isr()
 *
 * @brief This function is used to install the handling function for DW1000 IRQ.
 */
extern "C" void port_set_deca_isr(port_deca_isr_t deca_isr)
{
    port_deca_isr = deca_isr;
    
    if (deca_isr != nullptr) {
        pinMode(DW_IRQ_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(DW_IRQ_PIN), deca_isr, RISING);
    } else {
        detachInterrupt(digitalPinToInterrupt(DW_IRQ_PIN));
    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn reset_DW1000()
 *
 * @brief DW1000 reset function
 */
extern "C" void reset_DW1000(void)
{
    pinMode(DW_RST_PIN, OUTPUT);
    
    digitalWrite(DW_RST_PIN, LOW);
    delay(2);
    
    pinMode(DW_RST_PIN, INPUT);
    delay(2);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_dw1000_slowrate()
 *
 * @brief Set SPI rate to less than 3 MHz for initialisation
 */
extern "C" void port_set_dw1000_slowrate(void)
{
    extern void set_spi_speed(uint32_t speed_hz);
    set_spi_speed(3000000);  // 3 MHz for init
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_dw1000_fastrate()
 *
 * @brief Set SPI rate to 20 MHz after initialisation
 */
extern "C" void port_set_dw1000_fastrate(void)
{
    extern void set_spi_speed(uint32_t speed_hz);
    set_spi_speed(20000000); // 20 MHz for normal operation
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn Sleep()
 *
 * @brief Sleep delay in milliseconds
 */
extern "C" void Sleep(uint32_t ms)
{
    delay(ms);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn deca_sleep()
 *
 * @brief Sleep delay in milliseconds
 */
extern "C" void deca_sleep(unsigned int time_ms)
{
    delay(time_ms);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn deca_usleep()
 *
 * @brief Sleep delay in microseconds
 */
extern "C" void deca_usleep(unsigned long time_us)
{
    delayMicroseconds(time_us);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Mutex functions (not needed for single-threaded Arduino)
 */
extern "C" decaIrqStatus_t decamutexon(void)
{
    return 0;
}

extern "C" void decamutexoff(decaIrqStatus_t s)
{
    (void)s;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn peripherals_init()
 *
 * @brief Initialize peripherals
 */
extern "C" int peripherals_init(void)
{
    // SPI initialization
    openspi();
    
    // Reset DW1000
    reset_DW1000();
    
    return 0;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_wakeup_dw1000()
 *
 * @brief Wakeup DW1000 from sleep/deep sleep
 */
extern "C" void port_wakeup_dw1000(void)
{
    // Not implemented for this simple port
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_wakeup_dw1000_fast()
 *
 * @brief Fast wakeup DW1000 from sleep
 */
extern "C" void port_wakeup_dw1000_fast(void)
{
    // Not implemented for this simple port
}
