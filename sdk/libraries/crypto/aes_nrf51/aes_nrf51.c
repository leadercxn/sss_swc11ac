/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <string.h>
#include "aes.h"
#include "nrf_soc.h"

static nrf_ecb_hal_data_t m_aes_ecb;

return_type aes_set_key( const uint8_t key[], length_type keylen, aes_context ctx[1] )
{
    memcpy( m_aes_ecb.key, key, keylen );
    return 0;
}

return_type aes_encrypt( const uint8_t in[N_BLOCK], uint8_t  out[N_BLOCK], const aes_context ctx[1] )
{
    memcpy(m_aes_ecb.cleartext, in, 16);
    sd_ecb_block_encrypt(&m_aes_ecb);
    memcpy(out, m_aes_ecb.ciphertext, 16);
    return 0;
}
