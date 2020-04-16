#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "crc16.h"
//#include "softdevice_handler.h"
#include "pstorage.h"
//#include "app_scheduler.h"
//#include "storage.h"
#include "app_error.h"
#include "storage_handle.h"
#include "pstorage_platform.h"


static pstorage_handle_t            m_cmd_db_block_id;              //定义一个内存管理id
/**
 * 	储存模块初始化
 */
void storage_init(void)
{
    uint32_t err_code;
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);
}

/**
 *  获取存储模块的状态
 */
void storage_access_wait(void)
{
    uint32_t count = 1;
    do
    {
        pstorage_access_status_get(&count);
    }
    while(count != 0);
}
/**
 * 
 */
static void pstorage_ntf_cb(pstorage_handle_t  * p_handle,
                            uint8_t              op_code,
                            uint32_t             result,
                            uint8_t       *      p_data,
                            uint32_t             data_len)
{
    APP_ERROR_CHECK(result);
}

/**
 *  模块注册
 */
void storage_register(void)
{
    uint32_t err_code;
    pstorage_module_param_t pstorage_param;
    pstorage_param.cb = pstorage_ntf_cb;
    pstorage_param.block_size = 64;
    pstorage_param.block_count = 1;

    m_cmd_db_block_id.block_id = PSTORAGE_DATA_START_ADDR;
    err_code = pstorage_register(&pstorage_param, &m_cmd_db_block_id);
    APP_ERROR_CHECK(err_code);
}

/*
 *  命令的储存
 */
void storage_cmd_store(const char * cmd)
{
    uint32_t err_code;

    uint8_t cmd_buff[64] = {0};

    cmd_buff[0] = 1;

    strcpy((char *)&cmd_buff[1], cmd);

    err_code = pstorage_clear(&m_cmd_db_block_id, sizeof(cmd_buff));
    APP_ERROR_CHECK(err_code);

    err_code = pstorage_store(&m_cmd_db_block_id, cmd_buff, sizeof(cmd_buff), 0);
    APP_ERROR_CHECK(err_code);
}

