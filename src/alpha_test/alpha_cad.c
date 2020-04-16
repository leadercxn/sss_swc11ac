/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "alpha_cad.h"
//#include "sx1276-board.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "util.h"
#include "trace.h"
#include "app_timer.h"
#include "timer.h"

#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34


APP_TIMER_DEF(m_cad_timer);
static bool m_timer_created = false;

static uint32_t m_cad_interval = 2000;


static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnRxTimeout( void );
static void OnCadDone(bool channelActivityDetected);

static RadioEvents_t radioEvents =
{
    .TxDone = NULL,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = NULL,
    .RxTimeout = OnRxTimeout,
    .CadDone = OnCadDone
};

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep();

    printf("OnRxDone\r\n");
    print_dump(payload, size);
    Radio.StartCad();
//    timer_start(m_cad_timer, TIMER_CAD_INTERVAL);
}

static void OnRxTimeout( void )
{
    Radio.Sleep();
    Radio.StartCad();
    printf("OnRxTimeout\r\n");
}

static void OnCadDone(bool channelActivityDetected)
{
    printf( "OnCadDone: %d\r\n", channelActivityDetected);
    if(channelActivityDetected)
    {
        printf("start rx\r\n");
        Radio.Rx( 5500 );                           //设置接收超时时间
    }
    else
    {
        Radio.Sleep();
        TIMER_START(m_cad_timer, m_cad_interval);
    }
}

/**
 * 定时打开CAD检测
 */
static void cad_timer_handler(void *p_context)
{
    printf("StartCad\r\n");
    Radio.StartCad();
}

static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -f <float> target frequency in MHz, default 430\n" );
    printf( " -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n" );
    printf( " -s <uint>  Spreading Factor [6-12], default 11\n" );
    printf( " -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n" );
    printf( " -l <uint>  preamble length (symbols), default 8\n" );
    printf( " -t <uint>  symbol timeout (symbols), default 5\n" );
    printf( " -i         IQ invert\n" );
    printf( " -sy        Cfg sync word\n" );
    printf( " -nocrc     no crc\n");
    printf( " -fix <uint> fix payload len\n");
    printf( " -itv <uint> cad interval ms\n");
}


int alpha_cad_test( int argc, char **argv )
{
    uint8_t     count = 0;

    double freq      = 430;
    uint8_t bw      = 0;
    uint8_t sf      = 11;
    uint8_t cr      = 1;
    int pl      = 8;
    uint8_t st      = 5;
    bool sync       = false;
    bool invert     = false;
    bool crc        = false;
    bool fix_payload = false;
    uint8_t fix_payload_len = 0;

    uint32_t freq_int = 0;

    m_cad_interval = 2000;

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
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
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
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
            if( bw > 2 )
            {
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else if( strcmp( argv[i], "-s" ) == 0 )
        {
            i++;
            sf = atoi(argv[i]);
 #if 0
            count = sscanf( argv[i], "%u", ( unsigned int * )&sf );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
 #endif
            if( sf > 12 || sf < 6 )
            {
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
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
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
            if( cr > 4 || cr < 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
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
                printf( "subgcad: invalid arg\r\n" );
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
                printf( "subgcad: invalid arg\r\n" );
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
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
            fix_payload = true;
            i++;
        }
        else if( strcmp( argv[i], "-itv" ) == 0 )
        {
            i++;
            m_cad_interval = atoi(argv[i]);
            i++;
        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "subgcad: invalid arg\r\n" );
            return 0;
        }
    }
    freq_int = (uint32_t)(freq * 1e6);                  //freq初始值 430

    printf( "freq    = %d\r\n", freq_int );
    printf( "bw      = %d\r\n", bw );
    printf( "sf      = %d\r\n", sf );
    printf( "cr      = %d\r\n", cr );
    printf( "preamble length  = %d symb\r\n", pl );
    printf( "symbol timeout   = %d symb\r\n", st );
    printf( "sync    = %d\r\n", sync );
    printf( "invert  = %d\r\n", invert );
    printf( "crc     = %d\r\n", crc );
    printf( "fix len = %d\r\n", fix_payload_len );
    printf( "cat interval = %d\r\n", m_cad_interval );

    if(!m_timer_created)
    {
        m_timer_created = true;

        TIMER_CREATE(&m_cad_timer, APP_TIMER_MODE_SINGLE_SHOT, cad_timer_handler);
    }

    Radio.Init( &radioEvents );             //注册相关回调函数, OnRxDone , OnRxTimeout , OnCadDone.
    if( sync == true )
    {
        Radio.SetModem( MODEM_LORA );
        Radio.Write( REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD );
    }

    Radio.Sleep();

    Radio.SetChannel( freq_int );
    Radio.SetRxConfig( MODEM_LORA,
                       bw,
                       sf,
                       cr,
                       0,
                       pl,
                       st,
                       fix_payload,
                       fix_payload_len,
                       crc,
                       0,
                       0,
                       invert,
                       true );

    TIMER_START(m_cad_timer, m_cad_interval);

    printf( "subgcad: start, ctrl+c to stop\r\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            TIMER_STOP(m_cad_timer);
            Radio.Sleep();
            printf( "subgcad: end\r\n" );
            break;
        }
    }
    return 0;
}

