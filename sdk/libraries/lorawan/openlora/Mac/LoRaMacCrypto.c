/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: LoRa MAC layer implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <stdlib.h>
#include <string.h>
#include "cmac.h"
#include "LoRaMacCrypto.h"

/*!
 * CMAC/AES Message Integrity Code (MIC) Block B0 size
 */
#define LORAMAC_MIC_BLOCK_B0_SIZE                   16

/*!
 * MIC field computation initial data
 */
static uint8_t MicBlockB0[] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                              };

/*!
 * Contains the computed MIC field.
 *
 * \remark Only the 4 first bytes are used
 */
static uint8_t Mic[16];

/*!
 * Encryption aBlock and sBlock
 */
static uint8_t aBlock[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                          };
#ifndef USE_HW_ECB
static uint8_t sBlock[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                          };
#endif

/*!
 * AES computation context variable
 */
static aes_context AesContext;
                   
/*!
 * CMAC computation context variable
 */
static AES_CMAC_CTX AesCmacCtx[1];
                          
#ifdef USE_HW_ECB                          
static nrf_ecb_hal_data_t m_aes_ecb;
#endif                          

/*!
 * \brief Computes the LoRaMAC frame MIC field  
 *
 * \param [IN]  buffer          Data buffer
 * \param [IN]  size            Data buffer size
 * \param [IN]  key             AES key to be used
 * \param [IN]  address         Frame address
 * \param [IN]  dir             Frame direction [0: uplink, 1: downlink]
 * \param [IN]  sequenceCounter Frame sequence counter
 * \param [OUT] mic Computed MIC field
 */
void LoRaMacComputeMic( uint8_t *buffer, uint16_t size, uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint32_t *mic )
{
    MicBlockB0[5] = dir;
    
    MicBlockB0[6] = ( address ) & 0xFF;
    MicBlockB0[7] = ( address >> 8 ) & 0xFF;
    MicBlockB0[8] = ( address >> 16 ) & 0xFF;
    MicBlockB0[9] = ( address >> 24 ) & 0xFF;

    MicBlockB0[10] = ( sequenceCounter ) & 0xFF;
    MicBlockB0[11] = ( sequenceCounter >> 8 ) & 0xFF;
    MicBlockB0[12] = ( sequenceCounter >> 16 ) & 0xFF;
    MicBlockB0[13] = ( sequenceCounter >> 24 ) & 0xFF;

    MicBlockB0[15] = size & 0xFF;

    AES_CMAC_Init( AesCmacCtx );

    AES_CMAC_SetKey( AesCmacCtx, key );

    AES_CMAC_Update( AesCmacCtx, MicBlockB0, LORAMAC_MIC_BLOCK_B0_SIZE );
    
    AES_CMAC_Update( AesCmacCtx, buffer, size & 0xFF );
    
    AES_CMAC_Final( Mic, AesCmacCtx );
    
    *mic = ( uint32_t )( Mic[3] << 24 | Mic[2] << 16 | Mic[1] << 8 | Mic[0] );
}

void LoRaMacPayloadEncrypt( uint8_t *buffer, uint16_t size, uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *encBuffer )
{
    uint16_t i;
    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;

    memset( AesContext.ksch, '\0', 240 );

#ifdef USE_HW_ECB     
    memcpy(m_aes_ecb.key, key, 16);
#else
    aes_set_key( key, 16, &AesContext );   
#endif    

    aBlock[5] = dir;

    aBlock[6] = ( address ) & 0xFF;
    aBlock[7] = ( address >> 8 ) & 0xFF;
    aBlock[8] = ( address >> 16 ) & 0xFF;
    aBlock[9] = ( address >> 24 ) & 0xFF;

    aBlock[10] = ( sequenceCounter ) & 0xFF;
    aBlock[11] = ( sequenceCounter >> 8 ) & 0xFF;
    aBlock[12] = ( sequenceCounter >> 16 ) & 0xFF;
    aBlock[13] = ( sequenceCounter >> 24 ) & 0xFF;

    while( size >= 16 )
    {
        aBlock[15] = ( ( ctr ) & 0xFF );
        ctr++;

#ifdef USE_HW_ECB        
        memcpy(m_aes_ecb.cleartext, aBlock, 16 );
        sd_ecb_block_encrypt(&m_aes_ecb);
#else
        aes_encrypt( aBlock, sBlock, &AesContext );        
#endif        

        for( i = 0; i < 16; i++ )
        {
#ifdef USE_HW_ECB  
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ m_aes_ecb.ciphertext[i];
#else
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
#endif                        
        }
        size -= 16;
        bufferIndex += 16;
    }

    if( size > 0 )
    {
        aBlock[15] = ( ( ctr ) & 0xFF );
        
#ifdef USE_HW_ECB 
        memcpy(m_aes_ecb.cleartext, aBlock, 16 );
        sd_ecb_block_encrypt(&m_aes_ecb);
#else
        aes_encrypt( aBlock, sBlock, &AesContext );        
#endif        

        for( i = 0; i < size; i++ )
        {
#ifdef USE_HW_ECB 
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ m_aes_ecb.ciphertext[i];
#else
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
#endif            

            
        }
    }
}

void LoRaMacPayloadDecrypt( uint8_t *buffer, uint16_t size, uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *decBuffer )
{
    LoRaMacPayloadEncrypt( buffer, size, key, address, dir, sequenceCounter, decBuffer );
}

void LoRaMacJoinComputeMic( uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t *mic )
{
    AES_CMAC_Init( AesCmacCtx );

    AES_CMAC_SetKey( AesCmacCtx, key );

    AES_CMAC_Update( AesCmacCtx, buffer, size & 0xFF );

    AES_CMAC_Final( Mic, AesCmacCtx );

    *mic = ( uint32_t )( Mic[3] << 24 | Mic[2] << 16 | Mic[1] << 8 | Mic[0] );
}

void LoRaMacJoinDecrypt( uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *decBuffer )
{
#ifdef USE_HW_ECB
    memcpy(m_aes_ecb.key, key, 16);
    memcpy(m_aes_ecb.cleartext, buffer, 16 );
    sd_ecb_block_encrypt(&m_aes_ecb);
    memcpy(decBuffer, m_aes_ecb.ciphertext, 16);
#else
    memset1( AesContext.ksch, '\0', 240 );
    aes_set_key( key, 16, &AesContext );
    aes_encrypt( buffer, decBuffer, &AesContext );    
#endif    

    // Check if optional CFList is included
    if( size >= 16 )
    {
#ifdef USE_HW_ECB        
        memcpy(m_aes_ecb.cleartext, buffer + 16, 16 );
        sd_ecb_block_encrypt(&m_aes_ecb);
        memcpy(decBuffer + 16, m_aes_ecb.ciphertext, 16);
#else
        aes_encrypt( buffer + 16, decBuffer + 16, &AesContext );
#endif        
    }
}

void LoRaMacJoinComputeSKeys( const uint8_t *key, uint8_t *appNonce, uint16_t devNonce, uint8_t *nwkSKey, uint8_t *appSKey )
{
    uint8_t nonce[16];
    uint8_t *pDevNonce = ( uint8_t * )&devNonce;
    
#ifdef USE_HW_ECB   
    memcpy(m_aes_ecb.key, key, 16);
#else
    memset( AesContext.ksch, '\0', 240 );
    aes_set_key( key, 16, &AesContext );
#endif    
    
    memset( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x01;
    
    memcpy(nonce + 1, appNonce, 6);
    memcpy(nonce + 7, pDevNonce, 2);
//    LoRaMacMemCpy( appNonce, nonce + 1, 6 );
//    LoRaMacMemCpy( pDevNonce, nonce + 7, 2 );
 
#ifdef USE_HW_ECB 
    memcpy(m_aes_ecb.cleartext, nonce, 16 );
    sd_ecb_block_encrypt(&m_aes_ecb);
    memcpy(nwkSKey, m_aes_ecb.ciphertext, 16);    
#else
    aes_encrypt( nonce, nwkSKey, &AesContext );
#endif    
   
    memset( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x02;
    
    memcpy(nonce + 1, appNonce, 6);
    memcpy(nonce + 7, pDevNonce, 2);
//    LoRaMacMemCpy( appNonce, nonce + 1, 6 );
//    LoRaMacMemCpy( pDevNonce, nonce + 7, 2 );
    
    
#ifdef USE_HW_ECB  
    memcpy(m_aes_ecb.cleartext, nonce, 16 );
    sd_ecb_block_encrypt(&m_aes_ecb);
    memcpy(appSKey, m_aes_ecb.ciphertext, 16);
#else
    aes_encrypt( nonce, appSKey, &AesContext );
#endif
    
}

#ifdef USE_HW_ECB  
uint16_t LoRaMacPingOffsetGet(uint32_t beaconTime, uint32_t devaddr, uint16_t pingPeriod)
{    
    uint8_t rand[16];
    uint32_t beacon_time = beaconTime;
    uint32_t addr        = devaddr;
    
    memset(m_aes_ecb.key, 0, 16);
    memset(m_aes_ecb.cleartext, 0, 16);
    memcpy(m_aes_ecb.cleartext, &beacon_time, 4);
    memcpy(&m_aes_ecb.cleartext[4], &addr, 4);
    
//    trace_log("clear: ");
//    trace_dump(m_aes_ecb.cleartext, 16);
    
    sd_ecb_block_encrypt(&m_aes_ecb);
    memcpy(rand, m_aes_ecb.ciphertext, 16);
    
//    trace_log("cipher: ");
//    trace_dump(m_aes_ecb.ciphertext, 16);
//    uint16_t randOffset = 0;
//    randOffset = (rand[1]<<8) | rand[0];
//    trace_log("randOffset = %x, %d\r\n", randOffset, randOffset);
    
    return (((rand[1]<<8) | rand[0])& (pingPeriod-1));
}
#endif

