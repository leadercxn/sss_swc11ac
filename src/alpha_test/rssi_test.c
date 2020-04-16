#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "lwshell.h"
//#include "uart.h"
//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "trace.h"
#include "rssi_test.h"
#include "app_timer.h"
#include "timer.h"

APP_TIMER_DEF(m_rssi_timer);
static bool m_timer_created = false;


static void rssi_timer_handler(void *p_contex)
{
    int rssi = 0;
    rssi = SX1276ReadRssi( MODEM_LORA );
    printf( "rssi = %d\r\n", rssi);
}

static void usage(void)
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
//    printf( " -m <uint> mode, [0:FSK, 1:LORA]\n" );
    printf( " -f <uint> frequency , default 433.5Mhz \n" );
}
    
int rssi_test( int argc, char **argv )
{
    int rssi = 0;
    double freq = 433.5;
    uint32_t freq_int = 0;
//    uint8_t mode = 1;
    int count = 0;
    
    for( uint8_t i = 1; i < argc; )
    {
        trace_debug( "arg = %s\r\n", argv[i] );
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
                printf( "rssi: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
//        else if( strcmp( argv[i], "-m" ) == 0 )
//        {
//            i++;
//            mode = atoi( argv[i] );
//            if( mode > 1 )
//            {
//                printf( "rssi: invalid arg\r\n" );
//                return 0;
//            }
//            i++;
//        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "rssi: invalid arg\r\n" );
            return 0;
        }
    }
    
    freq_int = (uint32_t)(freq * 1e6);
    Radio.Init( NULL );
//    printf("mode = %d, [0:FSK, 1:LORA]\r\n", mode); 
    Radio.SetModem( MODEM_LORA );
    Radio.SetChannel( freq_int );
    SX1276SetOpMode( RF_OPMODE_RECEIVER );

    nrf_delay_ms(1);

    if(!m_timer_created)
    {
        m_timer_created = true;
        TIMER_CREATE(&m_rssi_timer, APP_TIMER_MODE_REPEATED, rssi_timer_handler);
    }
    rssi = SX1276ReadRssi( MODEM_LORA );
    
    TIMER_START(m_rssi_timer, 1000);
    printf( "rssi test: start, ctrl+c to stop\n\n" );
    printf("freq = %d\r\n", freq_int);
    printf( "rssi = %d\r\n", rssi);

    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            Radio.Sleep();
            printf( "rssi test : end\r\n" );
            TIMER_STOP(m_rssi_timer);
            break;
        }
    }

    return 0;
}
