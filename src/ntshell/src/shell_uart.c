#include <stdint.h>
#include <stdio.h>
#include "nrf.h"
#include "boards.h"
#include "app_uart.h"
#include "app_util_platform.h"

#define UART_TX_BUF_SIZE                        256
#define UART_RX_BUF_SIZE                        64

static void shell_uart_event_handler(app_uart_evt_t * p_event)
{
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            break;

        default:
            break;
    }
}

void shell_uart_init(void)
{
    uint32_t err_code;
    const app_uart_comm_params_t comm_params = {
        _UART_RX_PIN,
        _UART_TX_PIN,
        _UART_RTS_PIN,
        _UART_CTS_PIN,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 
                        shell_uart_event_handler, APP_IRQ_PRIORITY_LOW, err_code);
    APP_ERROR_CHECK(err_code);
}

void shell_uart_puts(char *str)
{
    while(*str)
    {
        app_uart_put(*str++);
    }
}

uint32_t shell_uart_getc(char *p_byte)
{
    return app_uart_get((uint8_t *)p_byte);
}

void shell_uart_putc(uint8_t c)
{
    app_uart_put(c);
}

