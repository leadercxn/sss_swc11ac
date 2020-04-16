#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ble_ctx.h"
#include "radio_config.h"
#include "nrf.h"
#include "trace.h"
//#include "uart.h"

static void usage(void)
{
    printf( "Available options: \n" );
    printf( " -h print this help \n" );
    printf( " -p <int>   RF power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4 \n" );
    printf( " -c <uint>  Channel, [0 - 100], default 0. Output 'frequency = 2400 + channel' MHz \n" );
}


/**@brief ble_ctx_test handler
 * 
 */
int ble_ctx_test(int argc, char **argv)
{
    uint8_t count = 0;

    int8_t txpower = 4;
    int8_t channel = 0;
    uint8_t mode = RADIO_MODE_MODE_Ble_1Mbit;

    ble_radio_config_t ctx_config;

    for( uint8_t i = 1; i < argc; )
    {
        if( strcmp( argv[i], "-h" ) == 0 )
        {
            i++;
            usage();
            return 0;
        }
        else if( strcmp( argv[i], "-p" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%d", ( int * )&txpower );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "blectx: invalid arg\r\n" );
                return 0;
            }
            if( txpower != -30 && txpower != -20 && txpower != -16 && txpower != -12 &&
                txpower != -8 && txpower != -4 && txpower != 0 && txpower != 4 )
            {
                trace_debug( "error\r\n" );
                printf( "blectx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            i++;
            channel = atoi( argv[i] );
            if(channel > 100)
            {
                printf( "blectx: invalid channel\r\n" );
                return 0;
            }
            i++;
        }
        else
        {
            printf( "blectx: invalid arg\r\n" );
            return 0;
        }
    }

    printf( "txpower   = %d\r\n", txpower );
    printf( "channel   = %d\r\n", channel );
    printf( "frequency = %d MHz\r\n", 2400 + channel );

    ctx_config.power   = txpower;
    ctx_config.channel = channel;
    ctx_config.mode    = mode;

    ble_radio_switch_init();
    ble_radio_const_carrier_configure();
    ble_radio_config_set(ctx_config);
    ble_radio_tx_enable();

    printf( "blectx: start, ctrl+c to stop \n\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            NVIC_ClearPendingIRQ(RADIO_IRQn);
            NVIC_DisableIRQ(RADIO_IRQn);

            ble_radio_disable();

            printf( "blectx test: end\r\n" );
            break;
        }
    }
    return false;
}
