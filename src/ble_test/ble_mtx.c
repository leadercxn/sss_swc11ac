
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nrf.h"
#include "ble_mtx.h"

static uint8_t packet[256];

/**
 * @brief Function for generating an 8 bit random number using the internal random generator.
*/
static uint32_t rnd8(void)
{
    NRF_RNG->TASKS_START = 1;
    NRF_RNG->EVENTS_VALRDY = 0;
    while(NRF_RNG->EVENTS_VALRDY == 0)
    {
        // Do nothing.
    }
    NRF_RNG->TASKS_STOP = 1;
    return  NRF_RNG->VALUE;
}


/**
 * @brief Function for generating a 32 bit random number using the internal random generator.
*/
static uint32_t rnd32(void)
{
    uint8_t i;
    uint32_t val = 0;

    for(i=0; i<4; i++)
    {
        val <<= 8;
        val |= rnd8();
    }
    return val;
}


/**
 * @brief Function for configuring the radio to use a random address and a 254 byte random payload.
 * The S0 and S1 fields are not used.
*/
static void generate_modulated_rf_packet(void)
{
    uint8_t i;

    NRF_RADIO->PREFIX0 = rnd8();
    NRF_RADIO->BASE0   = rnd32();

    // Packet configuration:
    // S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits
    NRF_RADIO->PCNF0  = (0UL << RADIO_PCNF0_S1LEN_Pos) |
                       (0UL << RADIO_PCNF0_S0LEN_Pos) |
                       (8UL << RADIO_PCNF0_LFLEN_Pos);
    // Packet configuration:
    // Bit 25: 1 Whitening enabled
    // Bit 24: 1 Big endian,
    // 4 byte base address length (5 byte full address length),
    // 0 byte static length, max 255 byte payload .
    NRF_RADIO->PCNF1  = (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |
                        (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) |
                        (4UL << RADIO_PCNF1_BALEN_Pos) |
                        (0UL << RADIO_PCNF1_STATLEN_Pos) |
                       (255UL << RADIO_PCNF1_MAXLEN_Pos);
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos);
    packet[0]         = 254;    // 254 byte payload.

    // Fill payload with random data.
    for(i = 0; i < 254; i++)
    {
        packet[i+1] = rnd8();
    }

    NRF_RADIO->PACKETPTR = (uint32_t)packet;
}


static void radio_disable(void)
{
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TEST            = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0)
    {
        // Do nothing.
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
}


/**
 * @brief Function for starting modulated TX carrier by repeatedly sending a packet with random address and
 * random payload.
*/
void radio_modulated_tx_carrier(uint8_t txpower, uint8_t mode, uint8_t channel)
{
    radio_disable();
    generate_modulated_rf_packet();
    NRF_RADIO->SHORTS     = RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_READY_START_Msk | \
                            RADIO_SHORTS_DISABLED_TXEN_Msk;;
    NRF_RADIO->TXPOWER    = (txpower << RADIO_TXPOWER_TXPOWER_Pos);
    NRF_RADIO->MODE       = (mode << RADIO_MODE_MODE_Pos);
    NRF_RADIO->FREQUENCY  = channel;
    NRF_RADIO->TASKS_TXEN = 1;
}

static void usage( void )
{
    printf( "Available options:\r\n" );
    printf( " -h print this help\r\n" );
    printf( " -p <int>   RF power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4\r\n" );
    printf( " -c <uint>  Channel, [0 - 100], default 0. Output 'frequency = 2400 + channel' MHz\r\n");
    printf( " -m <uint>  Mode, [0:Nrf_1Mbit, 1:Nrf_2Mbit, 2:Nrf_250Kbit, 3:Ble_1Mbit], default 3.\r\n" );
}

int ble_mtx_test(int argc, char **argv)
{
    int8_t txpower = 4;
    int8_t channel = 0;
    uint8_t mode = RADIO_MODE_MODE_Ble_1Mbit;

    for( uint8_t i = 1; i < argc; )
    {
        if( strcmp( argv[i], "-h" ) == 0 )
        {
            i++;
            usage();
            return false;
        }
        else if( strcmp( argv[i], "-p" ) == 0 )
        {
            i++;
            txpower = atoi( argv[i] );
            if( txpower != -30 && txpower != -20 && txpower != -16 && txpower != -12 &&
                txpower != -8 && txpower != -4 && txpower != 0 && txpower != 4 )
            {
                printf( "blemtx: invalid arg \n" );
                return false;
            }
            i++;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            i++;
            channel = atoi( argv[i] );
            if(channel > 100)
            {
                printf( "blemtx: invalid channel \n" );
                return false;
            }
            i++;

        }
        else if( strcmp( argv[i], "-m" ) == 0 )
        {
            i++;
            mode = atoi( argv[i] );
            if(mode > 3)
            {
                printf( "blemtx: invalid mode \n" );
                return false;
            }
            i++;

        }
        else
        {
            printf( "blemtx: invalid arg \n" );
            return false;
        }
    }

    printf( "mode      = %d \n", mode );
    printf( "txpower   = %d \n", txpower );
    printf( "channel   = %d \n", channel );
    printf( "frequency = %d MHz \n", 2400 + channel );

    radio_modulated_tx_carrier(txpower, mode, channel);

    printf( "blemtx: start, ctrl+c to stop \n\n" );
    while( 1 )
    {
        int c = getchar();
        if( c == 0x03 ) // ^C
        {
            radio_disable();
            printf( "blemtx test: end \n" );
            break;
        }
    }
    return false;
}

