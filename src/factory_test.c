#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//#include "nrf51.h"
#include "app_uart.h"
#include "boards.h"
#include "trace.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "util.h"
#include "ntshell.h"
#include "factory_test.h"
#include "storage_handle.h"
#include "shell_cmd.h"
#include "base_cmds.h"
#include "alpha_cmds.h"
#include "ble_cmds.h"
#include "pstorage_platform.h"

#include "nrf_gpio.h"

#define   UART_TX_BUF_SIZE    512          //UART TX buffer size
#define   UART_RX_BUF_SIZE    512          //UART RX buffer size

static ntshell_t  m_nts;
static void * mp_extobj = 0;

/**@brief Function for handling app_uart event callback.
 * format like this : typedef void (* app_uart_event_handler_t) (app_uart_evt_t * p_app_uart_event);
 * 
 */
static void app_uart_event_handler(app_uart_evt_t * p_app_uart_event)
{

}

/**
 *  串口初始化
 */
static void console_init(void)
{
    uint32_t err_code ;
    const app_uart_comm_params_t uart_params = 
    {
        _UART_RX_PIN ,
        _UART_TX_PIN ,
        _UART_RTS_PIN ,
        _UART_CTS_PIN , 
        APP_UART_FLOW_CONTROL_DISABLED ,
        false ,
        UART_BAUDRATE_BAUDRATE_Baud115200
    } ;

    APP_UART_FIFO_INIT( &uart_params, 
                        UART_RX_BUF_SIZE, 
                        UART_TX_BUF_SIZE, 
                        app_uart_event_handler, 
                        APP_IRQ_PRIORITY_LOW, 
                        err_code );
    APP_ERROR_CHECK(err_code);
  
    printf("\n" );
    //printf("tx ping = %d , rx_ping = %d ,rts_pin = %d ,cts_pin = %d \n",_UART_TX_PIN ,_UART_RX_PIN,_UART_RTS_PIN,_UART_CTS_PIN);
}

/**
 *  时钟初始化 
 */
static void clk_init(void)
{
    uint32_t delay = 0;
    uint32_t err_code;
    err_code = nrf_drv_clock_init();                    // Function for initializing the nrf_drv_clock module
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_hfclk_request(NULL);                  // Function for requesting the high-accuracy source HFCLK.
    while(!nrf_drv_clock_hfclk_is_running())
    {
        nrf_delay_ms(1);
        delay++;
        if(delay > 1000)
        {
            delay = 0;
            printf("hfclk init failed\r\n");
            break;
        }
    }
    nrf_drv_clock_lfclk_request(NULL);                  // Function for requesting the LFCLK. 
    while(!nrf_drv_clock_lfclk_is_running())
    {
        nrf_delay_ms(1);
        delay++;
        if(delay > 1000)
        {
            delay = 0;
            printf("lfclk init failed\r\n");
            break;
        }
    }
}

/**
 * 配置shell读函数
 */
static int shell_read(char * buf, int cnt, void * extobj)
{
    int tmp = 0;

    for(int i = 0; i < cnt; i++)
    {
        if(app_uart_get((uint8_t *)buf) == NRF_SUCCESS)
        {
            tmp++;
        }
    }
    return tmp;
}

/**
 * 配置shell写函数
 */
static int shell_write(const char * buf, int cnt, void * extobj)
{
    for(int i = 0; i < cnt; i++)
    {
        app_uart_put(buf[i]);
    }
    return cnt;
}

/**
 * shell回调
 */
static int shell_user_callback(const char * text, void * extobj)
{
    int rc = 0;

    rc = shell_cmd_execute(text);

    switch(rc)
    {
        case SHELL_ERROR_UNKNOWN_CMD:
            printf("error: unknown command found, ? for help\r\n");
            break;

        default:
            if(strlen(text) != 0)
            {
                if(!memcmp(text, "?", 1) ||
                   !strncmp(text, (char *)(PSTORAGE_DATA_START_ADDR + 1), strlen(text)))
                {
                    break;
                }
                storage_cmd_store(text);
            }
            break;
    }
    return 0;
}

#if 0
/**
 *  命令执行
 */
static void cached_cmd_execute(void)
{
    char last_cmd[64] = {0};
    static  uint8_t  m_cmd_cache = 0;
    uint8_t * p_last_cmd = (uint8_t *)PSTORAGE_DATA_START_ADDR;
    m_cmd_cache = *p_last_cmd;

    // printf("m_cmd_cache = %x\r\n", m_cmd_cache);
    if(m_cmd_cache == 1)
    {
        memcpy(last_cmd, p_last_cmd + 1, sizeof(last_cmd) - 1);

        if(is_buff_empty((uint8_t *)last_cmd, sizeof(last_cmd) - 1) || (strlen(last_cmd) == 0))
        {
            return;
        }

        uint8_t crlf[] = {0x0D, 0X0A};
        vtrecv_execute(&(m_nts.vtrecv), (unsigned char*)last_cmd, strlen(last_cmd));
        vtrecv_execute(&(m_nts.vtrecv), crlf, sizeof(crlf));
    }
}
#endif


/**
 *  shell inti
 */
static void shell_init(void)
{
    ntshell_init(&m_nts, shell_read, shell_write, shell_user_callback, mp_extobj);
    ntshell_set_prompt(&m_nts, ">>");
    ntshell_execute(&m_nts);
}

/**
 * shell 命令注册
 */
static void shell_cmds_register(void)
{
    base_cmds_register();
    alpha_cmds_register();
    ble_cmds_register();
}

/**@brief just printf the info
 *  
 */
static void info_message(void)
{
    printf("\n");
#ifdef VERSION_STR
    printf("Version v%s, compiled %s\r\n", VERSION_STR, __DATE__);
#endif
    printf("\n");
}
/**
 *  测试代码
 */
void factory_test(void)
{
    console_init() ;            //串口初始化 console 控制台初始化

    info_message() ; 
    
    clk_init();                 //因为没有初始化协议栈，所以需要初始化时钟

    storage_init() ;            //init the starage module

    storage_register() ;        //storage module register 

    shell_init() ;              //shell init 

    shell_cmds_register();      // shell register
    trace_debug("factory_test \n");
    while(1)
    {
        ntshell_execute_async(&m_nts);
    }
    
}












