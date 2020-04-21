#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble_serialization.h"
#include "alpha_ser_rx_dec.h"


/**
 * @brief 解析串口发送过来控制alpha rx参数的函数
 */
uint32_t  alpha_open_rx_req_dec(uint8_t const * const           p_buf,
                                uint32_t                        packet_len,
                                alpha_rx_param_t *              alpha_rx_param )
{
    SER_ASSERT_NOT_NULL(p_buf);
    SER_ASSERT_NOT_NULL(alpha_rx_param);

    uint32_t index = SER_CMD_DATA_POS;
    uint32_t err_code = NRF_SUCCESS;
/**
    |  Freq  |  BandWidth  |  DataRate(SF)  | CrcOn | iqInverted |
	|   4B   |     4B      |       4B       |   1B  |      1B    |
*/
    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->freq ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->bandwidth ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->datarate ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);
#if 0
    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->coderate ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint16_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->preambleLen ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint16_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->symbTimeout ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);
#endif
    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->crcOn ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &alpha_rx_param->iqInverted ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    SER_ASSERT_LENGTH_EQ(index, packet_len);
    return  err_code ;
}

/**
 * @brief 解析串口发送过来控制alpha tx参数的函数
 */
uint32_t  alpha_open_tx_req_dec(uint8_t const * const           p_buf,
                                uint32_t                        packet_len,
                                alpha_tx_param_t *              p_alpha_tx_param )
{
    SER_ASSERT_NOT_NULL(p_buf);
    SER_ASSERT_NOT_NULL(p_alpha_tx_param);

    uint32_t index = SER_CMD_DATA_POS;
    uint32_t err_code = NRF_SUCCESS;
/**
    |  Freq  | Power | BandWidth | DataRate(SF) | CrcOn | iqInverted |  txdata_len |	txbuff	|
	|   4B   |  1B   |   4B      |     4B       |   1B  |     1B     | 	   1B	   |	 nB 	|
 */
    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->freq ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    int8_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->power ));

    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->bandwidth ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint32_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->datarate ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->crcOn ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->iqInverted ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint8_t_dec(p_buf, packet_len, &index, (void *)( &p_alpha_tx_param->tx_buff_len ));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    memcpy( p_alpha_tx_param->tx_buff , &p_buf[index] , p_alpha_tx_param->tx_buff_len );          //数据拷贝

    index = index + p_alpha_tx_param->tx_buff_len ;

    SER_ASSERT_LENGTH_EQ(index, packet_len);
    return  err_code ;
}





