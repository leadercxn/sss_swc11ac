/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 */

#include "ble_radio_notification.h"
#include <stdlib.h>
#include "nrf_nvic.h"


//static bool                                 m_radio_active = false;  /**< Current radio state. */
static ble_radio_notification_evt_handler_t m_evt_handler  = NULL;   /**< Application event handler for handling Radio Notification events. */
static bool m_ble_radio_notificaion_status = false;

void SWI1_IRQHandler(void)
{
//    m_radio_active = !m_radio_active;
    if (m_evt_handler != NULL)
    {
//        m_evt_handler(m_radio_active);
        m_evt_handler();
    }
}


//uint32_t ble_radio_notification_init(uint32_t                             irq_priority,
//                                     nrf_radio_notification_distance_t    distance,
//                                     ble_radio_notification_evt_handler_t evt_handler)
//{
//    uint32_t err_code;

//    m_evt_handler = evt_handler;

//    // Initialize Radio Notification software interrupt
//    err_code = sd_nvic_ClearPendingIRQ(SWI1_IRQn);
//    if (err_code != NRF_SUCCESS)
//    {
//        return err_code;
//    }

//    err_code = sd_nvic_SetPriority(SWI1_IRQn, irq_priority);
//    if (err_code != NRF_SUCCESS)
//    {
//        return err_code;
//    }

//    err_code = sd_nvic_EnableIRQ(SWI1_IRQn);
//    if (err_code != NRF_SUCCESS)
//    {
//        return err_code;
//    }

//    // Configure the event
//    return sd_radio_notification_cfg_set(NRF_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH, distance);
//}

uint32_t ble_radio_notification_init(uint32_t                             irq_priority,
                                     ble_radio_notification_evt_handler_t evt_handler)
{
    uint32_t err_code;

    m_evt_handler = evt_handler;

    // Initialize Radio Notification software interrupt
    err_code = sd_nvic_ClearPendingIRQ(SWI1_IRQn);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = sd_nvic_SetPriority(SWI1_IRQn, irq_priority);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return err_code;
}

uint32_t ble_radio_notification_on(void)
{
    uint32_t err_code;
    
    if(!m_ble_radio_notificaion_status)
    {
        err_code = sd_nvic_EnableIRQ(SWI1_IRQn);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        
        // Configure the event
        err_code = sd_radio_notification_cfg_set(NRF_RADIO_NOTIFICATION_TYPE_INT_ON_INACTIVE, NRF_RADIO_NOTIFICATION_DISTANCE_5500US);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        m_ble_radio_notificaion_status = true;
    }
    return NRF_SUCCESS;
}

uint32_t ble_radio_notification_off(void)
{
    uint32_t err_code;
    
    if(m_ble_radio_notificaion_status)
    {
        err_code = sd_nvic_DisableIRQ(SWI1_IRQn);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        
        // Configure the event
        err_code = sd_radio_notification_cfg_set(NRF_RADIO_NOTIFICATION_TYPE_NONE, NRF_RADIO_NOTIFICATION_DISTANCE_5500US);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        m_ble_radio_notificaion_status = false;   
    }
    return NRF_SUCCESS;
}

bool ble_radio_notification_status_get(void)
{
    return m_ble_radio_notificaion_status;
}

