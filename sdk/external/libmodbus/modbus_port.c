/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "modbus_port.h"

__WEAK void modbus_rtu_port_connect(int buad)
{

}

__WEAK void modbus_rtu_port_send(uint8_t *req, uint16_t req_length)
{

}

__WEAK int modbus_rtu_port_recv(uint8_t *rsp, int rsp_length, int timeout)
{
	return 0;
}

__WEAK void modbus_rtu_port_close(void)
{

}

__WEAK void modbus_debug(char *str, ...)
{

}
