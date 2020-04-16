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
#include "math.h"

#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34


APP_TIMER_DEF(m_cad_timer);
static bool m_timer_created = false;

static uint32_t  m_cad_interval = 2000;             //cad开启的时间间隔
static uint8_t   m_channels_scan = 8 ;              //扫描的通道数
static int8_t    m_current_scan_channel = 0;        //当前正在扫描的通道
static uint8_t   mbw      = 1;
static uint8_t   msf      = 11;
static uint8_t   mcr      = 1;
static int       mpl      = 8;
static uint8_t   mst      = 5; 
static bool      mfix_payload = false;
static uint8_t   mfix_payload_len = 0;
static bool      mcrc      = false;
static bool      msync     = false;
static bool      minvert   = false;
static uint32_t  mfreq_int = 0;

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnRxTimeout( void );
static void OnCadDone(bool channelActivityDetected);

/**
 * @brief config the API
 */
static RadioEvents_t radioEvents =
{
    .TxDone    = NULL,
    .RxDone    = OnRxDone,
    .RxError   = NULL,
    .TxTimeout = NULL,
    .RxTimeout = OnRxTimeout,
    .CadDone   = OnCadDone
};

/**
 * @brief set rx RxConfig
 */
static void freq_set_and_cad_start(void)
{
    uint32_t freq_offset = 0 ;
    freq_offset = m_current_scan_channel*pow(2,mbw)*125*1e3 ;

    Radio.SetChannel( mfreq_int + freq_offset );
    Radio.StartCad();

}


static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep();

    print_dump(payload, size);
    m_current_scan_channel++ ; 
    if(m_current_scan_channel > (m_channels_scan - 1))
    {
        m_current_scan_channel = 0 ;
    }
    freq_set_and_cad_start();
}

static void OnRxTimeout( void )
{
    Radio.Sleep();
    m_current_scan_channel++ ; 
    if(m_current_scan_channel > (m_channels_scan - 1))
    {
        m_current_scan_channel = 0 ;
    }
    freq_set_and_cad_start();
    printf("OnRxTimeout\r\n");
}

static void OnCadDone(bool channelActivityDetected)
{
    uint32_t freq_offset = 0 ;
    freq_offset = m_current_scan_channel*pow(2,mbw)*125*1e3 ;

    if(channelActivityDetected)
    {
        printf("rch = %d , f = %d \r\n" , m_current_scan_channel , (mfreq_int + freq_offset));
        printf("rx data result >>>> ");
        Radio.Rx( 5500 );                           //设置接收超时时间
    }
    else
    {
        Radio.Sleep();
        
        m_current_scan_channel++ ; 
        if(m_current_scan_channel > (m_channels_scan - 1))
        {
            m_current_scan_channel = 0 ;
        }
        freq_set_and_cad_start() ;
    }
}

/**
 * 定时打开CAD检测
 */
static void cad_timer_handler(void *p_context)
{
    #if 0
    uint32_t freq_offset = 0 ;
    freq_offset = m_current_scan_channel*pow(2,mbw)*125*1e3 ;

    Radio.SetChannel( mfreq_int + freq_offset );
    Radio.SetRxConfig( MODEM_LORA,
                       mbw,
                       msf,
                       mcr,
                       0,
                       mpl,
                       mst,
                       mfix_payload,
                       mfix_payload_len,
                       mcrc,
                       0,
                       0,
                       minvert,
                       true );
    printf("StartCad -------------------------------------------\n");
    printf("current chananel = %d , freq = %d \n" , m_current_scan_channel , (mfreq_int + freq_offset));
    Radio.StartCad();

    m_current_scan_channel++ ; 
    if(m_current_scan_channel > (m_channels_scan - 1))
    {
        m_current_scan_channel = 0 ;
    }
    #endif
}

static void usage( void )
{
    printf( "Available options:\n" );
    printf( " -h print this help\n" );
    printf( " -f <float> init frequency in MHz, default 430\n" );
    printf( " -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 1:250\n" );
    printf( " -s <uint>  Spreading Factor [6-12], default 11\n" );
    printf( " -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n" );
    printf( " -l <uint>  preamble length (symbols), default 8\n" );
    printf( " -t <uint>  symbol timeout (symbols), default 5\n" );
    printf( " -i         IQ invert\n" );
    printf( " -sy        Cfg sync word\n" );
    printf( " -nocrc     no crc\n");
    printf( " -fix <uint> fix payload len\n");
    printf( " -itv <uint> cad interval ms\n");
    printf( " -sc <uint> number of scanning channel [1-40]\n");
}


int alpha_multi_channel_cad_test( int argc, char **argv )
{
    uint8_t     count = 0;

    double freq     = 430;
    uint8_t bw      = 1;
//    uint8_t msf      = 11;
    uint8_t cr      = 1;
    int pl          = 8;
    uint8_t st      = 5;
//    bool msync       = false;
//    bool minvert     = false;
//    bool mcrc        = false;
//    bool fix_payload = false;
    uint8_t fix_payload_len = 0;
    uint8_t channels_scan = 8 ;

//    uint32_t mfreq_int = 0;
    m_cad_interval = 2000;
    m_current_scan_channel = 0 ;                        //每一次配置测试，都要重新清0当前的测试通道

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
            msf = atoi(argv[i]);
            if( msf > 12 || msf < 6 )
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
            minvert = true;
        }
        else if( strcmp( argv[i], "-sy" ) == 0 )
        {
            i++;
            msync = true;
        }
        else if( strcmp( argv[i], "-nocrc" ) == 0 )
        {
            i++;
            mcrc = false;
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
            mfix_payload = true;
            i++;
        }
        else if( strcmp( argv[i], "-itv" ) == 0 )
        {
            i++;
            m_cad_interval = atoi(argv[i]);
            i++;
        }
        else if( strcmp( argv[i], "-sc" ) == 0 )
        {
            i++;
            count = sscanf( argv[i], "%u", ( unsigned int * )&channels_scan );
            if( count != 1 )
            {
                trace_debug( "error\r\n" );
                printf( "subgcad: invalid arg\r\n" );
                return 0;
            }
            if(( channels_scan > 40 )||( channels_scan < 1 ))
            {
                trace_debug( "error\r\n" );
                printf( "subgmccad: invalid arg\r\n" );
                return 0;
            }
            i++;
        }
        else
        {
            trace_debug( "error\r\n" );
            printf( "subgcad: invalid arg\r\n" );
            return 0;
        }
    }

    mfreq_int = (uint32_t)(freq * 1e6);                  //freq初始值 430
    mbw  =  bw  ;                                        //不直接使用堆中的全局变量，是因为全局变量在sscanf中使用起来存在BUG。
    mcr  =  cr  ;
    mpl  =  pl  ;
    mst  =  st  ;
    mfix_payload_len  =  fix_payload_len ;
    m_channels_scan   =  channels_scan ;

    printf( "freq    = %d\r\n", mfreq_int );
    printf( "bw      = %d\r\n", mbw );
    printf( "sf      = %d\r\n", msf );
    printf( "cr      = %d\r\n", mcr );
    printf( "preamble length  = %d symb\r\n", mpl );
    printf( "symbol timeout   = %d symb\r\n", mst );
    printf( "sync    = %d\r\n", msync );
    printf( "invert  = %d\r\n", minvert );
    printf( "crc     = %d\r\n", mcrc );
    printf( "fix len = %d\r\n", mfix_payload_len );
    printf( "cat interval  = %d\r\n", m_cad_interval );
    printf( "scan channels = %d\r\n", m_channels_scan );

    if(!m_timer_created)
    {
        m_timer_created = true;

        TIMER_CREATE(&m_cad_timer, APP_TIMER_MODE_SINGLE_SHOT, cad_timer_handler);
    }

    Radio.Init( &radioEvents );                          //注册相关回调函数, OnRxDone , OnRxTimeout , OnCadDone.
    if( msync == true )
    {
        Radio.SetModem( MODEM_LORA );
        Radio.Write( REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD );
    }

    Radio.Sleep();

    Radio.SetRxConfig( MODEM_LORA,
                       mbw,
                       msf,
                       mcr,
                       0,
                       mpl,
                       mst,
                       mfix_payload,
                       mfix_payload_len,
                       mcrc,
                       0,
                       0,
                       minvert,
                       true );

    freq_set_and_cad_start();                            // 配置RX config ，并开始cad的接收

    //   TIMER_START( m_cad_timer, m_cad_interval );

    printf( "subgmccad: start, ctrl+c to stop\r\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            TIMER_STOP(m_cad_timer);
            Radio.Sleep();
            printf( "subgmccad: end\r\n" );
            break;
        }
    }
    return 0;
}



