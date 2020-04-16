#ifndef __BOARD_ALARM_HOST_V100_H
#define __BOARD_ALARM_HOST_V100_H


#define _SX127X_RESET           4

#define _SX127X_DIO_0           5
#define _SX127X_DIO_1           6

#define _SX127X_ANT_TX          17
#define _SX127X_ANT_RX          18

#define _SPIM_SS_PIN            16
#define _SPIM_MOSI_PIN          15
#define _SPIM_MISO_PIN          14
#define _SPIM_SCK_PIN           13

#define _UART_TX_PIN  			1
#define _UART_RX_PIN  			0
#define _UART_CTS_PIN 			32
#define _UART_RTS_PIN 			32
#define _UART_HWFC           	false
#define _UART_BAUD              115200

#define _UART_CFG_TX_PIN        10
#define _UART_CFG_RX_PIN  	    20
#define _UART_CFG_CTS_PIN 		32
#define _UART_CFG_RTS_PIN 		32
#define _UART_CFG_HWFC          false
#define _UART_CFG_BAUD          115200

#define SER_CON_RX_PIN          _UART_RX_PIN      
#define SER_CON_TX_PIN          _UART_TX_PIN
#define SER_CON_CTS_PIN         _UART_CTS_PIN
#define SER_CON_RTS_PIN         _UART_RTS_PIN




#endif



