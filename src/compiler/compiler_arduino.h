/*! ----------------------------------------------------------------------------
 * @file    compiler_arduino.h
 * @brief   Compiler definitions for Arduino
 */

#ifndef COMPILER_ARDUINO_H_
#define COMPILER_ARDUINO_H_

#ifdef __cplusplus
extern "C" {
#endif

// Packed attribute for structures
#ifndef __packed
#define __packed __attribute__((packed))
#endif

// Weak attribute for functions
#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMPILER_ARDUINO_H_ */
