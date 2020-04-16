/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __MODBUS_PORT_H__
#define __MODBUS_PORT_H__

#if defined ( __CC_ARM )

    #ifndef __WEAK
        #define __WEAK              __weak
    #endif

#elif defined ( __ICCARM__ )

    #ifndef __WEAK
        #define __WEAK              __weak
    #endif

#elif defined   ( __GNUC__ )

    #ifndef __WEAK
        #define __WEAK              __attribute__((weak))
    #endif

#endif

/**
 * [modbus_rtu_port_connect description]
 *
 * @param    buad    [description]
 */
void modbus_rtu_port_connect(int buad);

/**
 * [modbus_rtu_port_send description]
 *
 * @param    req           [description]
 * @param    req_length    [description]
 */
void modbus_rtu_port_send(uint8_t *req, uint16_t req_length);

/**
 * [modbus_rtu_port_recv description]
 *
 * @param     rsp           [description]
 * @param     rsp_length    [description]
 * @param     timeout       [description]
 *
 * @return                  [description]
 */
int modbus_rtu_port_recv(uint8_t *rsp, int rsp_length, int timeout);

/**
 * [modbus_rtu_port_close description]
 */
void modbus_rtu_port_close(void);

/**
 *
 */
void modbus_debug(char *str, ...);


#endif
