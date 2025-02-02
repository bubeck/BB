/*
 * debug.h
 *
 *  Created on: Apr 17, 2020
 *      Author: horinek
 */

#ifndef DEBUG_THREAD_H_
#define DEBUG_THREAD_H_

#include "common.h"

#define DBG_DEBUG	0
#define DBG_INFO	1
#define DBG_WARN	2
#define DBG_ERROR	3
#define DBG_RAW		0xFF

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DBG_INFO
#endif

#define FAULT(...)  debug_fault(__VA_ARGS__);

#if DEBUG_LEVEL <= DBG_DEBUG
#define DBG(...)    debug_send(DBG_DEBUG, __VA_ARGS__)
#define RAW(...)    debug_send(DBG_RAW, __VA_ARGS__)
#define DUMP(data, len) debug_dump(data, len)
#else
#define DBG(...)
#define RAW(...)
#define DUMP(data, len)
#endif


#if DEBUG_LEVEL <= DBG_INFO
#define INFO(...)	debug_send(DBG_INFO, __VA_ARGS__)
#else
#define INFO(...)
#endif

#if DEBUG_LEVEL <= DBG_WARN
#define WARN(...)	debug_send(DBG_WARN, __VA_ARGS__)
#else
#define WARN(...)
#endif

#if DEBUG_LEVEL <= DBG_ERROR
#define ERR(...)	debug_send(DBG_ERROR, __VA_ARGS__)
#else
#define ERR(...)
#endif

#define __DEBUG_BKPT()  asm volatile ("bkpt 0")

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILENAME__
#endif

#define ASSERT(cond)	\
	do {	\
		if (!(cond))	\
		{ \
			ERR("Assertion failed %s:%u", __FILE_NAME__, __LINE__); \
		} \
	} while(0);


#define FASSERT(cond)    \
    do {    \
        if (!(cond))    \
        { \
            bsod_msg("Assertion failed %s:%u", __FILE_NAME__, __LINE__); \
        } \
    } while(0);


void debug_fault(const char *format, ...);
void debug_fault_dump(uint8_t * data, uint32_t len);

void debug_send(uint8_t type, const char *format, ...);
void debug_dump(uint8_t * data, uint16_t len);
void debug_uart_done();

uint16_t debug_get_waiting();
uint8_t debug_read_byte();
void debug_read_bytes(uint8_t * buff, uint16_t len);

void thread_debug_start(void *argument);
void debug_uart_error();
void debug_uart_start_dma();

extern bool debug_thread_ready;

#endif /* DEBUG_THREAD_H_ */
