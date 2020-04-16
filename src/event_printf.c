#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "user_cfg.h"

//
char *app_uart_fifo_event_log_str(nrf_drv_uart_evt_type_t evt_type)
{
    static char *event_str[]=
    {
        "NRF_DRV_UART_EVT_TX_DONE",
        "NRF_DRV_UART_EVT_RX_DONE",
        "NRF_DRV_UART_EVT_ERROR",
    };
    return event_str[evt_type];
}


//
char *phy_uart_event_log_str(app_uart_evt_type_t evt_type)
{
    static char *event_str[]=
    {
        "APP_UART_DATA_READY",
        "APP_UART_FIFO_ERROR",
        "APP_UART_COMMUNICATION_ERROR",
        "APP_UART_TX_EMPTY",
        "APP_UART_DATA",
    };
    return event_str[evt_type];
}

//
char *ser_phy_event_log_str(ser_phy_evt_type_t evt_type)
{
    static char *event_str[]=
    {
        "SER_PHY_EVT_TX_PKT_SENT"        ,   /**< TX包已被传输的事件 */
		"SER_PHY_EVT_RX_BUF_REQUEST"     ,   /**< 表明物理层需要一个RX包的缓冲区。应该阻塞一个PHY流，直到调用@ref ser_phy_rx_buf_set函数 */
		"SER_PHY_EVT_RX_PKT_RECEIVED"    ,   /**< 数据包成功接收 */
		"SER_PHY_EVT_RX_PKT_DROPPED"     ,   /**< 接收过载，导致数据丢失 */
		"SER_PHY_EVT_RX_OVERFLOW_ERROR"  ,   /**< 写过载 */
		"SER_PHY_EVT_TX_OVERREAD_ERROR"  ,   /**< 读过载 */
		"SER_PHY_EVT_HW_ERROR"           ,   /**< 硬件错误  */
		"SER_PHY_EVT_TYPE_MAX"           ,   /**< 枚举上线 */
    };
    return event_str[evt_type];
}

//
char *ser_hal_event_log_str(ser_hal_transport_evt_type_t evt_type)
{
    static char *event_str[]=
    {
        "SER_HAL_TRANSP_EVT_TX_PKT_SENT"           ,
		"SER_HAL_TRANSP_EVT_RX_PKT_RECEIVING"      , 
		"SER_HAL_TRANSP_EVT_RX_PKT_RECEIVED"       ,
		"SER_HAL_TRANSP_EVT_RX_PKT_DROPPED"        ,
		"SER_HAL_TRANSP_EVT_PHY_ERROR"             ,
		"SER_HAL_TRANSP_EVT_TYPE_MAX"              ,
    };
    return event_str[evt_type];
}




