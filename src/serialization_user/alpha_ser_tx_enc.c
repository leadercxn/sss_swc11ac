#include "ble_serialization.h"
#include "ser_alpha.h"

#include "alpha_ser_tx_enc.h"













uint32_t alpha_alive_rsp_enc(uint32_t         return_code,
                             uint8_t * const  p_buf,
                             uint32_t * const p_buf_len)
{
    uint32_t index = 0;

    return op_status_enc(ALPHA_ALIVE_ACCESS, return_code, p_buf, p_buf_len, &index);
}




