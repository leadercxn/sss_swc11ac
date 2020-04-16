#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "alpha_fhtx.h"

//#include "lwshell.h"
//#include "uart.h"
//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
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
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  true
#define ALPHA_RX_IQ_INVERSION_ON                     false
#define ALPHA_TX_IQ_INVERSION_ON                     false
#define ALPHA_NB_SYMB_HOP                            100

static uint8_t m_buff[255];
static uint8_t m_buff_len = 20;
static uint16_t m_count   = 0;

static int m_repeat          = -1;
static int m_delay           = 0;
static uint8_t m_bw       = 0;
static uint8_t m_sf       = 11;
static uint8_t m_cr       = 1;
static uint8_t m_pl       = 8;
static int m_power        = 14;
static bool m_invert      = false;
static bool m_fix         = false;
static uint16_t m_hp       = 4;

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

//const uint32_t HoppingFrequencies[] =
//{
//    470300000,
//    470500000,
//    470700000,
//    470900000,
//    471300000,
//    471500000,
//    471700000,
//    471900000,
//};

static void OnTxDone( void )
{
    Radio.SetChannel( HoppingFrequencies[0] );
    Radio.Sleep();
    printf( "OnTxDone\r\n\r\n" );
    m_count++;
    printf( "TX: %d\r\n", m_count );

    m_buff[1] = m_count >> 8;
    m_buff[2] = m_count & 0xff;

    nrf_delay_ms( m_delay );

    Radio.SetTxConfig( MODEM_LORA, m_power, 0, m_bw,
                       m_sf, m_cr,
                       m_pl, m_fix,
                       true, true, m_hp, m_invert, 3000000 );

    if( m_count < m_repeat || m_repeat == -1 )
    {
        Radio.Send( m_buff, m_buff_len );
    }
}

static void OnTxTimeout( void )
{
    Radio.SetChannel( HoppingFrequencies[0] );
    printf( "OnTxTimeout\r\n" );
    m_count++;
    printf( "TX: %d\r\n", m_count );

    m_buff[1] = m_count >> 8;
    m_buff[2] = m_count & 0xff;

    nrf_delay_ms( m_delay );

    if( m_count < m_repeat || m_repeat == -1 )
    {
        Radio.Send( m_buff, m_buff_len );
    }
}

static void OnFhssChangeChannel(uint8_t channelIndex)
{
    Radio.SetChannel( HoppingFrequencies[channelIndex] );
    trace_info("channelIndex = %d\r\n", channelIndex);
}

static RadioEvents_t TxTestEvents =
{
    .TxDone = OnTxDone,
    .RxDone = NULL,
    .RxError = NULL,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = NULL,
    .CadDone = NULL,
    .FhssChangeChannel = OnFhssChangeChannel
};


static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -f <float> target frequency in MHz, default 433.5\n" );
    printf( " -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n" );
    printf( " -s <uint>  Spreading Factor [6-12], default 11\n" );
    printf( " -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n" );
    printf( " -p <int>   RF power (dBm),default 14\n" );
    printf( " -l <uint>  preamble length (symbols), default 8\n" );
    printf( " -i <uint>  IQ invert\n" );
    printf( " -sy Cfg sync word\n" );
    printf( " -i  IQ invert\n" );
    printf( " -t <uint>  pause between packets (ms),default 0\n" );
    printf( " -r <uint>  repeat times, (-1 loop until stopped,default -1)\n" );
    printf( " -bl <uint> tx buffer len, default 20\n" );
    printf( " -id <uint> set a id, default 0\n" );
    printf( " -fix <uint> fix payload len\n");
    printf( " -hp <uint> hopping period\n");
}



/**@brief
 *
 * @param
 *
 * @retval
 */
int alpha_fhtx_test( int argc, char **argv )
{
    uint8_t     count = 0;
    float freq        = 433.5;
    bool sync         = false;
    uint8_t id        = 0;


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
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-b" ) == 0 )
        {
            i++;
            m_bw = atoi( argv[i] );
            if( m_bw > 2 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-s" ) == 0 )
        {
            i++;
            m_sf = atoi( argv[i] );
            if( m_sf > 12 || m_sf < 6 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-p" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%d", ( unsigned int * )&m_power );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            if( m_power > 20 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            i++;
            m_cr = atoi( argv[i] );
            if( m_cr > 4 || m_cr < 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-l" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&m_pl );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-t" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&m_delay );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-r" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%d", ( unsigned int * )&m_repeat );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-bl" ) == 0 )
        {
            i++;
            m_buff_len = atoi( argv[i] );
            if( m_buff_len < 5 || m_buff_len > 255 )
            {
                trace_debug( "error\r\n" );
                printf( "subgfhtx: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-i" ) == 0 )
        {
            i++;
            m_invert = true;
        }
        else if( strcmp( argv[i], "-sy" ) == 0 )
        {
            i++;
            sync = true;
        }
        else if( strcmp( argv[i], "-id" ) == 0 )
        {
            i++;
            sscanf( argv[i], "%u", ( unsigned int * )&id );
            i++;
        }
        else if( strcmp( argv[i], "-fix" ) == 0 )
        {
            i++;
            m_fix = true;
        }
        else if( strcmp( argv[i], "-hp" ) == 0 )
        {
            i++;
            m_hp = atoi( argv[i] );
            i++;
        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "subgfhtx: invalid arg\r\n" );
            return 0;
        }
    }
    printf( "freq    = %u\r\n", HoppingFrequencies[0] );
    printf( "bw      = %u\r\n", m_bw );
    printf( "sf      = %u\r\n", m_sf );
    printf( "power   = %d\r\n", m_power );
    printf( "cr      = %u\r\n", m_cr );
    printf( "pl      = %u symb\r\n", m_pl );
    printf( "sync    = %u\r\n", sync );
    printf( "invert  = %d\r\n", m_invert );
    printf( "delay   = %u\r\n", m_delay );
    printf( "repeat  = %d\r\n", m_repeat );
    printf( "tx len  = %u\r\n", m_buff_len );
    printf( "id      = %02x\r\n", id );
    printf( "fix     = %d\r\n", m_fix );
    printf( "hp      = %u\r\n", m_hp );

    m_count = 0;
    m_buff[0] = id;
    m_buff[1] = m_count >> 8;
    m_buff[2] = m_count & 0xff;
    for( uint8_t i = 3; i < m_buff_len; i++ )
    {
        m_buff[i] = i;
    }


    Radio.Init( &TxTestEvents );
    if( sync == true )
    {
        Radio.SetModem( MODEM_LORA );
        Radio.Write( REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD );
    }

    Radio.Sleep();

    Radio.SetChannel( HoppingFrequencies[0] );
//    Radio.SetChannel( freq * 1000000 );

    printf( "subgfhtx: start, ctrl+c to stop\r\n" );

    printf( "TX: %d\r\n", m_count );
    Radio.SetTxConfig( MODEM_LORA, m_power, 0, m_bw,
                       m_sf, m_cr,
                       m_pl, m_fix,
                       true, true, m_hp, m_invert, 3000 );

    Radio.Send( m_buff, m_buff_len );


    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            Radio.Sleep();
            printf( "subgfhtx test: end\r\n" );
            break;
        }
    }

    return 0;
}


