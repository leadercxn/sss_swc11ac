#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "alpha_drx.h"
//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"
#include "run_time.h"

#include "trace.h"


#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34

#define ALPHA_BANDWIDTH                              0           // [0: 125 kHz,
                                                                //  1: 250 kHz,
                                                                //  2: 500 kHz,
                                                                //  3: Reserved]
#define ALPHA_SPREADING_FACTOR                       11          // [SF7..SF12]
#define ALPHA_CODINGRATE                             1           // [1: 4/5,
                                                                //  2: 4/6,
                                                                //  3: 4/7,
                                                                //  4: 4/8]
#define ALPHA_SYMBOL_TIMEOUT                         5           // Symbols
#define ALPHA_PREAMBLE_LENGTH                        8           // Same for Tx and Rx
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  false
#define ALPHA_RX_IQ_INVERSION_ON                     false
#define ALPHA_TX_IQ_INVERSION_ON                     false


#define ALPHADRX_STATE_UPLINK    0
#define ALPHADRX_STATE_DOWNLINK  1


static uint32_t uplink_freq_int = 0;
static uint32_t downlink_freq_int = 0;

static uint8_t uplink_sf = 11;
static uint8_t downlink_sf = 11;

static uint8_t bw      = 0;
static uint8_t cr      = 1;
static uint8_t pl      = 8;
static uint8_t st      = 5;
static bool sync       = false;
static bool fix_payload = false;
static uint8_t fix_payload_len = 0;

static uint8_t drx_state = ALPHADRX_STATE_UPLINK;

static bool run_time_started = false;

static uint32_t uplink_timestamp = 0;

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    uint8_t buffer[256] = {0};

    memcpy(buffer, payload, size);

    if(drx_state == ALPHADRX_STATE_UPLINK)
    {
        drx_state = ALPHADRX_STATE_DOWNLINK;

        uplink_timestamp = RunTimeGet();

        Radio.Sleep();
        Radio.SetChannel( downlink_freq_int );
        Radio.SetRxConfig( MODEM_LORA,
                           bw,
                           downlink_sf,
                           cr,
                           0,
                           pl,
                           st,
                           fix_payload,
                           fix_payload_len,
                           false,
                           0,
                           0,
                           true,
                           true );
        Radio.Rx( 3000 );

        printf( "uplink: ");
        print_dump( buffer, size );
        printf( "RSSI = %d, SNR = %d\r\n\r\n", rssi, snr );
    }
    else
    {
        uint32_t downlink_timestamp = 0;

        drx_state = ALPHADRX_STATE_UPLINK;

        downlink_timestamp = RunTimeGet();

        Radio.Sleep();
        Radio.SetChannel( uplink_freq_int );
        Radio.SetRxConfig( MODEM_LORA,
                           bw,
                           uplink_sf,
                           cr,
                           0,
                           pl,
                           st,
                           fix_payload,
                           fix_payload_len,
                           true,
                           0,
                           0,
                           false,
                           true );
        Radio.Rx( 0 );

        printf( "downlink: ");
        print_dump( buffer, size );
        printf( "RSSI = %d, SNR = %d\r\n", rssi, snr );
        printf("timestamp diff = %u\r\n\r\n", downlink_timestamp - uplink_timestamp);
    }
}

static void OnRxTimeout( void )
{
    printf( "downlink timeout\r\n\r\n");

    Radio.Sleep();

    if(drx_state == ALPHADRX_STATE_DOWNLINK)
    {
        drx_state = ALPHADRX_STATE_UPLINK;

        Radio.SetChannel( uplink_freq_int );
        Radio.SetRxConfig( MODEM_LORA,
                           bw,
                           uplink_sf,
                           cr,
                           0,
                           pl,
                           st,
                           fix_payload,
                           fix_payload_len,
                           true,
                           0,
                           0,
                           false,
                           true );
        Radio.Rx( 0 );
    }
    else
    {
        printf( "invalid state\r\n");
    }
}

static RadioEvents_t RxTestEvents =
{
    .TxDone = NULL,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = NULL,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL
};

static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -uf <float> target uplink   frequency in MHz, default 433\n" );                    //上行频点
    printf( " -df <float> target downlink frequency in MHz, default 433\n" );                    //下行频点
    printf( " -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n" );
    printf( " -us <uint> Uplink Spreading Factor [6-12], default 11\n" );                        //上行速度
    printf( " -ds <uint> Downlink  Spreading Factor [6-12], default 11\n" );                     //下行速度
    printf( " -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n" );
    printf( " -l <uint>  preamble length (symbols), default 8\n" );
    printf( " -t <uint>  symbol timeout (symbols), default 5\n" );
    printf( " -i         IQ invert\n" );
    printf( " -sy        Cfg sync word\n" );
    printf( " -fix <uint> fix payload len\n");
}

int alpha_drx_test( int argc, char **argv )
{
    uint8_t arg_cnt = 0;
    double uplink_freq = 433;
    double downlink_freq = 433;

    arg_cnt = 0;
    for( uint8_t i = 1; i < argc; )
    {
        trace_debug( "arg = %s\r\n", argv[i] );
        if( strcmp( argv[i], "-h" ) == 0 )
        {
            i++;
            usage();
            return 0;
        }
        else if( strcmp( argv[i], "-uf" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%lf", &uplink_freq );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-df" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%lf", &downlink_freq );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-b" ) == 0 )
        {
            i++;

            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&bw );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            if( bw > 2 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-us" ) == 0 )
        {
            i++;
            uplink_sf = atoi(argv[i]);
//            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&uplink_sf );
//            if( arg_cnt != 1 )
//            {
//                trace_debug( "error\r\n" );
//                printf( "alphadrx: invalid arg\r\n" );
//                return 0;
//            }
            if( uplink_sf > 12 || uplink_sf < 6 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-ds" ) == 0 )
        {
            i++;

            downlink_sf = atoi(argv[i]);
//            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&downlink_sf );
//            if( arg_cnt != 1 )
//            {
//                trace_debug( "error\r\n" );
//                printf( "alphadrx: invalid arg\r\n" );
//                return 0;
//            }
            if( downlink_sf > 12 || downlink_sf < 6 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&cr );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            if( cr > 4 || cr < 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-l" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&pl );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-t" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&st );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }

            i++;
        }
        else if( strcmp( argv[i], "-sy" ) == 0 )
        {
            i++;
            sync = true;
        }
        else if( strcmp( argv[i], "-fix" ) == 0 )
        {
            i++;
            arg_cnt = sscanf( argv[i], "%u", ( unsigned int * )&fix_payload_len );
            if( arg_cnt != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "alphadrx: invalid arg\r\n" );
                return 0;
            }
            fix_payload = true;
            i++;
        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "alphadrx: invalid arg\r\n" );
            return 0;
        }
    }
    uplink_freq_int = (uint32_t)(uplink_freq * 1e6);
    downlink_freq_int = (uint32_t)(downlink_freq * 1e6);

    printf( "ufreq    = %u\r\n", uplink_freq_int );
    printf( "dfreq    = %u\r\n", downlink_freq_int );
    printf( "bw      = %d\r\n", bw );
    printf( "usf      = %d\r\n", uplink_sf );
    printf( "dsf      = %d\r\n", downlink_sf );
    printf( "cr      = %d\r\n", cr );
    printf( "preamble length  = %d symb\r\n", pl );
    printf( "symbol timeout   = %d symb\r\n", st );
    printf( "sync    = %d\r\n", sync );
    printf( "fix len = %d\r\n", fix_payload_len );

    if(!run_time_started)
    {
        run_time_started = true;

        RunTimeInit();
    }

    Radio.Init( &RxTestEvents );
    if( sync == true )
    {
        Radio.SetModem( MODEM_LORA );
        Radio.Write( REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD );
    }

    drx_state = ALPHADRX_STATE_UPLINK;

    Radio.Sleep();

    Radio.SetChannel( uplink_freq_int );
    Radio.SetRxConfig( MODEM_LORA,
                       bw,
                       uplink_sf,
                       cr,
                       0,
                       pl,
                       st,
                       fix_payload,
                       fix_payload_len,
                       true,
                       0,
                       0,
                       false,
                       true );
    Radio.Rx( 0 );

    printf( "alphadrx: start, ctrl+c to stop\n\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            Radio.Sleep();
            printf( "alphadrx test: end\n\n" );
            break;
        }
    }
    return 0;
}

