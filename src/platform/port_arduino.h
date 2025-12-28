/*! ----------------------------------------------------------------------------
 * @file    port_arduino.h
 * @brief   HW specific definitions and functions for Arduino ESP32-C3
 */

#ifndef PORT_ARDUINO_H_
#define PORT_ARDUINO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

/* DW1000 IRQ handler type */
typedef void (*port_deca_isr_t)(void);

/* DW1000 IRQ handler declaration */
extern port_deca_isr_t port_deca_isr;

/* Type definitions for compatibility */
typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;

/* IRQ status type for mutex */
typedef int decaIrqStatus_t;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* Function declarations */
void port_set_deca_isr(port_deca_isr_t deca_isr);
void reset_DW1000(void);
void port_set_dw1000_slowrate(void);
void port_set_dw1000_fastrate(void);
void Sleep(uint32_t ms);
void deca_sleep(unsigned int time_ms);
void deca_usleep(unsigned long time_us);
decaIrqStatus_t decamutexon(void);
void decamutexoff(decaIrqStatus_t s);
int peripherals_init(void);
void port_wakeup_dw1000(void);
void port_wakeup_dw1000_fast(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_ARDUINO_H_ */
