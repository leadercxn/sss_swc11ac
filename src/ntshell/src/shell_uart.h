#ifndef SHELL_UART_H__
#define SHELL_UART_H__

void shell_uart_init(void);
void shell_uart_puts(char *str);
uint32_t shell_uart_getc(char *p_byte);
void shell_uart_putc(uint8_t c);

#endif
