#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "alpha_ctx.h"
//#include "lwshell.h"
//#include "uart.h"
//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "trace.h"
#include "util.h"

//extern uint32_t g_cmd_count;


/**@brief
 */
static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -f <float> target frequency in MHz, default 433\n" );
    printf( " -p <int>   RF power (dBm),default 14\n" );
//    printf( " -rfo use rfo output pin\n");
}

/**@brief
 *
 * @param
 *
 * @retval
 */
int alpha_ctx_test(int argc, char **argv)
{
    uint8_t     count = 0;
    double freq  = 433;
    int power   = 14;
//    uint8_t pa_select = RF_PACONFIG_PASELECT_PABOOST;    //新的SDK有所修改，所以屏蔽

    uint32_t freq_int = 0;

    for( uint8_t i = 1; i < argc; )
    {
        if( strcmp( argv[i], "-h" ) == 0 )
        {
            i++;
            usage();
            return 0;
        }
        else if( strcmp( argv[i], "-f" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%lf", &freq );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgctx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-p" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", &power );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgctx: invalid arg\r\n" );
                return 0;
            }
            i++;
            if( power > 20 )
            {
                trace_debug( "error\r\n" );
                printf( "subgctx: invalid arg\r\n" );
                return 0;
            }
        }
//        else if( strcmp( argv[i], "-rfo" ) == 0 )
//        {
//            i++;
//            pa_select = RF_PACONFIG_PASELECT_RFO;
//        }
        else
        {
            printf( "subgctx: invalid arg\r\n" );
            return 0;
        }
    }

//    if(g_cmd_count > 0)
//    {
//        NVIC_SystemReset();
//    }

    freq_int = (uint32_t)(freq * 1e6);

    printf( "freq    = %d\r\n", freq_int );
    printf( "power   = %d\r\n", power );
//    printf( "pacfg   = %02X\r\n", pa_select );

    Radio.Init( NULL );
//    Radio.SetPaSelect(pa_select);

    Radio.SetTxContinuousWave(freq_int, power, 0);
    printf( "subgctx: start, ctrl+c to stop\n\n" );

    uint8_t reg = Radio.Read( REG_IRQFLAGS1 );
    printf("PLL Locked = %d\n\n", ((reg & 0x10) != 0)); 
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            Radio.Sleep();
            printf("subgctx test : end \n\n");
            break;
        }
    }

    return 0;
}
