#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "alpha_fhrx.h"

//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"
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


static uint16_t count;

static const uint32_t HoppingFrequencies[] =
{
    916500000,
    923500000,
    906500000,
    917500000,
    917500000,
    909000000,
    903000000,
    916000000,
    912500000,
    926000000,
    925000000,
    909500000,
    913000000,
    918500000,
    918500000,
    902500000,
    911500000,
    926500000,
    902500000,
    922000000,
    924000000,
    903500000,
    913000000,
    922000000,
    926000000,
    910000000,
    920000000,
    922500000,
    911000000,
    922000000,
    909500000,
    926000000,
    922000000,
    918000000,
    925500000,
    908000000,
    917500000,
    926500000,
    908500000,
    916000000,
    905500000,
    916000000,
    903000000,
    905000000,
    915000000,
    913000000,
    907000000,
    910000000,
    926500000,
    925500000,
    911000000,
};

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.SetChannel( HoppingFrequencies[0] );
    count++;
    printf( "OnRxDone %03d: ", count );
    print_dump( payload, size );
    printf( "RSSI = %d, SNR = %d\r\n", rssi, snr );
}

static void OnRxTimeout( void )
{
    printf("OnRxTimeout\r\n");
    Radio.SetChannel( HoppingFrequencies[0] );

}

static void OnFhssChangeChannel(uint8_t channelIndex)
{
    Radio.SetChannel( HoppingFrequencies[channelIndex] );
    trace_info("channelIndex = %d\r\n", channelIndex);
}

static RadioEvents_t RxTestEvents =
{
    .TxDone = NULL,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = NULL,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL,
    .FhssChangeChannel = OnFhssChangeChannel
};

static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -f <float> target frequency in MHz, default 433\n" );
    printf( " -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n" );
    printf( " -s <uint>  Spreading Factor [6-12], default 11\n" );
    printf( " -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n" );
    printf( " -l <uint>  preamble length (symbols), default 8\n" );
    printf( " -t <uint>  symbol timeout (symbols), default 5\n" );
    printf( " -i         IQ invert\n" );
    printf( " -sy        Cfg sync word\n" );
    printf( " -nocrc     no crc\n");
    printf( " -fix <uint> fix payload len\n");
}

int alpha_fhrx_test( int argc, char **argv )
{
    uint8_t     count = 0;
    float freq      = 433;
    uint8_t bw      = 0;
    uint8_t sf      = 11;
    uint8_t cr      = 1;
    uint8_t pl      = 8;
    uint8_t st      = 5;
    bool sync       = false;
    bool invert     = false;
    bool crc        = true;
    bool fix_payload = false;
    uint16_t hp     = 4;
    uint8_t fix_payload_len = 0;

    count = 0;

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
            count = sscanf( argv[i], "%f", &freq );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-b" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&bw );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            if( bw > 2 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-s" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&sf );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            if( sf > 12 || sf < 6 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&cr );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            if( cr > 4 || cr < 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-l" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&pl );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-t" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&st );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }

            i++;
        }
        else if( strcmp( argv[i], "-i" ) == 0 )
        {
            i++;
            invert = true;
        }
        else if( strcmp( argv[i], "-sy" ) == 0 )
        {
            i++;
            sync = true;
        }
        else if( strcmp( argv[i], "-nocrc" ) == 0 )
        {
            i++;
            crc = false;
        }
        else if( strcmp( argv[i], "-fix" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&fix_payload_len );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhrx: invalid arg\r\n" );
                return 0;
            }
            fix_payload = true;
            i++;
        }
                else if( strcmp( argv[i], "-hp" ) == 0 )
        {
            i++;

            hp = atoi( argv[i] );
            i++;
        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "subgfhrx: invalid arg\r\n" );
            return 0;
        }
    }
    printf( "freq    = %u\r\n", HoppingFrequencies[0] );
    printf( "bw      = %d\r\n", bw );
    printf( "sf      = %d\r\n", sf );
    printf( "cr      = %d\r\n", cr );
    printf( "preamble length  = %d symb\r\n", pl );
    printf( "symbol timeout   = %d symb\r\n", st );
    printf( "sync    = %d\r\n", sync );
    printf( "invert  = %d\r\n", invert );
    printf( "crc     = %d\r\n", crc );
    printf( "fix len = %d\r\n", fix_payload_len );
    printf( "hp      = %u\r\n", hp );

    Radio.Init( &RxTestEvents );
    if( sync == true )
    {
        Radio.SetModem( MODEM_LORA );
        Radio.Write( REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD );
    }

    Radio.Sleep();

//    Radio.SetChannel( freq * 1000000 );
    Radio.SetChannel( HoppingFrequencies[0] );
    Radio.SetRxConfig( MODEM_LORA,      //mode
                       bw,               //bandwidth
                       sf,              //datarate
                       cr,               //coderate
                       0,               //bandwidthAfc
                       pl,               //preambleLen
                       255,               //symbTimeout
                       fix_payload,      //fixLen
                       fix_payload_len, //payloadLen
                       crc,             //crcOn
                       true,            //FreqHopOn
                       hp,              //HopPeriod
                       invert,          //iqInverted
                       true );          //rxContinuous
    Radio.Rx( 0 );

    printf( "subgfhrx: start, ctrl+c to stop\n\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            Radio.Sleep();
            printf( "subgfhrx test: end\n" );
            break;
        }
    }
    return 0;
}

