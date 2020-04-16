/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __TRACE_H_
#define __TRACE_H_

#include <stdint.h>
#include <stdio.h>

#ifdef TRACE_ENABLE
#include "nrf_log.h"

#define TRACE_LEVEL_ASSERT                      0
#define TRACE_LEVEL_ERROR                       1
#define TRACE_LEVEL_WARN                        2
#define TRACE_LEVEL_NOTICE                      3
#define TRACE_LEVEL_INFO                        4
#define TRACE_LEVEL_DEBUG                       5
#define TRACE_LEVEL_VERBOSE                     6

#ifndef TRACE_LEVEL
#define TRACE_LEVEL TRACE_LEVEL_DEBUG
#endif

#ifndef __MODULE__
#define __MODULE__ "undefined"
#endif

#ifndef TRACE_ASSERT_FORMAT
#define TRACE_ASSERT_FORMAT                     "%-10s\t%4d [A] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_ERROR_FORMAT
#define TRACE_ERROR_FORMAT                      "%-10s\t%4d [E] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_WARN_FORMAT
#define TRACE_WARN_FORMAT                       "%-10s\t%4d [W] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_NOTICE_FORMAT
#define TRACE_NOTICE_FORMAT                     "%-10s\t%4d [N] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_INFO_FORMAT
#define TRACE_INFO_FORMAT                       "%-10s\t%4d [I] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_DEBUG_FORMAT
#define TRACE_DEBUG_FORMAT                      "%-10s\t%4d [D] ", __MODULE__, __LINE__
#endif

#ifndef TRACE_VERBOSE_FORMAT
#define TRACE_VERBOSE_FORMAT                    "%-10s\t%4d [V] ", __MODULE__, __LINE__
#endif

void trace_init(void);

void trace_dump(void * p_buffer, uint32_t len);

#define TRACE_PRINTF    NRF_LOG_PRINTF

#if TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#define trace_assert(msg, ...)  {TRACE_PRINTF(TRACE_ASSERT_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_a            trace_dump
#else
#define trace_assert(msg, ...)
#define trace_dump_a(...)
#endif

#if TRACE_LEVEL >= TRACE_LEVEL_ERROR
#define trace_error(msg, ...)   {TRACE_PRINTF(TRACE_ERROR_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_e            trace_dump
#else
#define trace_error(msg, ...)
#define trace_dump_e(...)
#endif

#if TRACE_LEVEL >= TRACE_LEVEL_WARN
#define trace_warn(msg, ...)    {TRACE_PRINTF(TRACE_WARN_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_w            trace_dump
#else
#define trace_warn(msg, ...)
#define trace_dump_w(...)
#endif

#if TRACE_LEVEL >= TRACE_LEVEL_NOTICE
#define trace_notice(msg, ...)  {TRACE_PRINTF(TRACE_NOTICE_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_n            trace_dump
#else
#define trace_warn(msg, ...)
#define trace_dump_n(...)
#endif

#if TRACE_LEVEL >= TRACE_LEVEL_INFO
#define trace_info(msg, ...)    {TRACE_PRINTF(TRACE_INFO_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_i            trace_dump
#else
#define trace_info(msg, ...)
#define trace_dump_i(...)
#endif

#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
#define trace_debug(msg, ...)   {TRACE_PRINTF(TRACE_DEBUG_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_d            trace_dump
#else
#define trace_debug(msg, ...)
#define trace_dump_d(...)
#endif

#if TRACE_LEVEL == TRACE_LEVEL_VERBOSE
#define trace_verbose(msg, ...) {TRACE_PRINTF(TRACE_VERBOSE_FORMAT);TRACE_PRINTF(msg, ##__VA_ARGS__);}
#define trace_dump_v            trace_dump
#else
#define trace_verbose(msg, ...)
#define trace_dump_v(...)
#endif

#else

#define trace_init(...)
#define trace_printf(...)
#define trace_dump(...)

#define trace_assert(...)
#define trace_dump_a(...)

#define trace_error(...)
#define trace_dump_e(...)

#define trace_warn(...)
#define trace_dump_w(...)

#define trace_notice(...)
#define trace_dump_n(...)

#define trace_info(...)
#define trace_dump_i(...)

#define trace_debug(...)
#define trace_dump_d(...)

#define trace_verbose(...)
#define trace_dump_v(...)

#endif


#endif
