#ifndef __USER_CFG_H_
#define __USER_CFG_H_

#define USING_LOG_IN_SERIALIZATION
#define USING_LOG_IN_LORA

#include "trace.h"

#if defined(USING_LOG_IN_SERIALIZATION)     //start  #if defined(USING_LOG_IN_SERIALIZATION) 

#define LOG_I       trace_info    
#define LOG_HEX_I   trace_dump

#define LOG_E       trace_error    
#define LOG_HEX_E   trace_dump
#else

#define LOG_I(...)       
#define LOG_HEX_I(...) 

#endif                                      //end  #if defined(USING_LOG_IN_SERIALIZATION) 
















#include "nrf_drv_uart.h"
char *app_uart_fifo_event_log_str(nrf_drv_uart_evt_type_t evt_type);

#include "app_uart.h"
char *phy_uart_event_log_str(app_uart_evt_type_t evt_type);

#include "ser_phy.h"
char *ser_phy_event_log_str(ser_phy_evt_type_t evt_type);

#include "ser_hal_transport.h"
char *ser_hal_event_log_str(ser_hal_transport_evt_type_t evt_type);






#endif

