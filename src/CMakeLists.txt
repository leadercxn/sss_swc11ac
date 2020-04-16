set(target "sss_swc11ac")

#set(FW_TYPE "FIRMWARE" CACHE STRING "Type of firmware for build")
set(FW_TYPE "DEBUG" CACHE STRING "Type of firmware for build")
set_property(CACHE FW_TYPE PROPERTY STRINGS "DEBUG" "FIRMWARE" "RELEASE")

set(TRACE_LEVEL "TRACE_LEVEL_DEBUG" CACHE STRING "Type of trace level")
set_property(CACHE TRACE_LEVEL PROPERTY STRINGS "TRACE_LEVEL_ASSERT" "TRACE_LEVEL_ERROR" 
                "TRACE_LEVEL_WARN" "TRACE_LEVEL_NOTICE" "TRACE_LEVEL_INFO" "TRACE_LEVEL_DEBUG" "TRACE_LEVEL_VERBOSE")

set(NRF_LOG_BACKEND "RTT" CACHE STRING "Type of nrf log backend")
#set(NRF_LOG_BACKEND "UART" CACHE STRING "Type of nrf log backend")
set_property(CACHE NRF_LOG_BACKEND PROPERTY STRINGS "RTT" "UART" "RAW_UART")

message(STATUS "firmware type: ${FW_TYPE}")
message(STATUS "trace type: ${TRACE_LEVEL}")
message(STATUS "nrf log backend: ${NRF_LOG_BACKEND}")

##################################################################
##################################################################
# Add app source files
file(GLOB APP_SOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/lib/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/lib/util/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/alpha_test/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ble_test/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/base_test/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/storage/*.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_user/*.c"

#    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport/ser_phy/ser_phy.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport/ser_phy/ser_phy_nrf51_uart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport/ser_hal_transport.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130/ble_gap_struct_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130/ble_gattc_struct_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130/ble_gatts_struct_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130/ble_struct_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130/nrf_soc_struct_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/ble_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/cond_field_serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_handlers.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_cmd_decoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_dtm_cmd_decoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_pkt_decoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_event_encoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/ser_conn_reset_cmd_decoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/hal/dtm_uart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/common/ble_dtm_init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/common/conn_mw.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_event.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_evt_tx_complete.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_evt_user_mem_release.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_evt_user_mem_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_adv_report.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_auth_key_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_auth_status.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_conn_param_update.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_conn_param_update_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_conn_sec_update.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_connected.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_disconnected.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_key_pressed.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_lesc_dhkey_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_passkey_display.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_rssi_changed.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_scan_req_report.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_sec_info_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_sec_params_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_sec_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_timeout.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_attr_info_disc_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_char_disc_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_char_val_by_uuid_read_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_char_vals_read_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_desc_disc_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_hvx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_prim_srvc_disc_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_read_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_rel_disc_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_timeout.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_evt_write_rsp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_hvc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_rw_authorize_request.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_sc_confirm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_sys_attr_missing.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_timeout.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_evt_write.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_l2cap_evt_rx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_enable.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_opt_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_opt_id_pre_decoder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_opt_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_tx_packet_count_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_user_mem_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_uuid_decode.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_uuid_encode.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_uuid_vs_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_version_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/conn_ble_user_mem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_address_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_address_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_adv_data_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_adv_start.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_adv_stop.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_appearance_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_appearance_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_auth_key_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_authenticate.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_conn_sec_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_connect_cancel.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_connect.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_device_name_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_device_name_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_disconnect.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_encrypt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_key_pressed.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_keypress_notify.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_lesc_dhkey_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_lesc_oob_data_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_lesc_oob_data_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_ppcp_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_ppcp_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_rssi_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_rssi_start.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_rssi_stop.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_scan_start.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_scan_stop.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_sec_info_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_sec_params_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_tx_power_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_attr_info_discover.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_char_value_by_uuid_read.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_char_values_read.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_characteristics_discover.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_descriptors_discover.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_hv_confirm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_primary_services_discover.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_read.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_relationships_discover.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gattc_write.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_attr_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_attr_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_characteristic_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_descriptor_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_hvx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_include_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_initial_user_handle_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_rw_authorize_reply.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_service_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_service_changed.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_sys_attr_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_sys_attr_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_value_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gatts_value_set.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_l2cap_cid_register.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_l2cap_cid_unregister.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_l2cap_tx.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ecb_block_encrypt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/power_system_off.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/temp_get.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/ble_gap_evt_conn_param_update.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/conn_ble_gap_sec_keys.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/power_system_off.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/power_system_off.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers/power_system_off.c"


    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_items.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_ble.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_ble_gap.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_ble_gattc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_ble_gatts.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_ble_l2cap.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware/conn_mw_nrf_soc.c"

)

set(APP_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/lib/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/ntshell/lib/util"
    "${CMAKE_CURRENT_SOURCE_DIR}/alpha_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/ble_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/base_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/storage"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_user"

    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/struct_ser/s130"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport/ser_phy"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/serializers"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/codecs/s130/middleware"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/connectivity/hal"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization_lib/common/transport/ser_phy/config"
    
)

#####################################################
#####################################################
#Add sdk files
set(SDK_SOURCE_FILES
    "${SDK_ROOT}/libraries/nrf51/run_time/run_time.c"

)

set(SDK_INCLUDE_DIRS
    "${SDK_ROOT}/libraries/nrf51/run_time"

)
#####################################################3
#####################################################


set(nRF5_SDK_11_SOURCE_FILES
    "${NRF_SDK_ROOT}/components/libraries/bootloader_dfu/bootloader_util.c"
    "${NRF_SDK_ROOT}/components/libraries/button/app_button.c"
    "${NRF_SDK_ROOT}/components/libraries/crc16/crc16.c"
    "${NRF_SDK_ROOT}/components/libraries/fifo/app_fifo.c"
    "${NRF_SDK_ROOT}/components/libraries/util/app_util_platform.c"
#    "${NRF_SDK_ROOT}/components/libraries/scheduler/app_scheduler.c"
    "${NRF_SDK_ROOT}/components/libraries/scheduler/app_scheduler_serconn.c"
    "${NRF_SDK_ROOT}/components/libraries/timer/app_timer.c"
    "${NRF_SDK_ROOT}/components/libraries/timer/app_timer_appsh.c"
    "${NRF_SDK_ROOT}/components/libraries/uart/app_uart_fifo.c"    
#    "${NRF_SDK_ROOT}/components/libraries/uart/app_uart.c"
    "${NRF_SDK_ROOT}/components/libraries/uart/retarget.c"                                  #printf 关联
    "${NRF_SDK_ROOT}/components/libraries/util/app_error.c"
    "${NRF_SDK_ROOT}/components/libraries/util/nrf_assert.c"
    "${NRF_SDK_ROOT}/components/libraries/util/nrf_log.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/common/nrf_drv_common.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/delay/nrf_delay.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/gpiote/nrf_drv_gpiote.c"
#    "${NRF_SDK_ROOT}/components/drivers_nrf/pstorage/pstorage_raw.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/pstorage/pstorage_nosd.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/ble_flash/ble_flash.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/timer/nrf_drv_timer.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/uart/nrf_drv_uart.c"
    "${NRF_SDK_ROOT}/components/drivers_nrf/clock/nrf_drv_clock.c"
    "${NRF_SDK_ROOT}/components/softdevice/common/softdevice_handler/softdevice_handler_appsh.c"
    "${NRF_SDK_ROOT}/components/softdevice/common/softdevice_handler/softdevice_handler.c"
    "${NRF_SDK_ROOT}/components/ble/common/ble_srv_common.c"
    "${NRF_SDK_ROOT}/components/ble/ble_dtm/ble_dtm.c"
)

set(nRF5_SDK_11_INCLUDE_DIRS
    "${NRF_SDK_ROOT}/components/device"
    "${NRF_SDK_ROOT}/components/drivers_nrf/delay"
    "${NRF_SDK_ROOT}/components/drivers_nrf/uart"
    "${NRF_SDK_ROOT}/components/drivers_nrf/pstorage"
    "${NRF_SDK_ROOT}/components/drivers_nrf/ble_flash"
    "${NRF_SDK_ROOT}/components/drivers_nrf/common"
    "${NRF_SDK_ROOT}/components/drivers_nrf/gpiote"
    "${NRF_SDK_ROOT}/components/drivers_nrf/hal"
    "${NRF_SDK_ROOT}/components/drivers_nrf/clock"
    "${NRF_SDK_ROOT}/components/libraries/timer"
    "${NRF_SDK_ROOT}/components/libraries/fifo"
    "${NRF_SDK_ROOT}/components/libraries/util"
    "${NRF_SDK_ROOT}/components/libraries/uart"
    "${NRF_SDK_ROOT}/components/libraries/button"
    "${NRF_SDK_ROOT}/components/libraries/scheduler"
    "${NRF_SDK_ROOT}/components/libraries/experimental_section_vars"
    "${NRF_SDK_ROOT}/components/libraries/hardfault"
    "${NRF_SDK_ROOT}/components/libraries/bootloader_dfu"
    "${NRF_SDK_ROOT}/components/libraries/crc16"
    "${NRF_SDK_ROOT}/components/softdevice/s130/headers"
    "${NRF_SDK_ROOT}/components/softdevice/s130/headers/nrf51"
    "${NRF_SDK_ROOT}/components/softdevice/common/softdevice_handler"
    "${NRF_SDK_ROOT}/components/ble/common"
    "${NRF_SDK_ROOT}/components/ble/ble_dtm"
)

##############################################################
##############################################################

if(NRF_LOG_BACKEND MATCHES "RTT")
    set(nRF5_SDK_11_SOURCE_FILES ${nRF5_SDK_11_SOURCE_FILES}
        "${NRF_SDK_ROOT}/external/segger_rtt/SEGGER_RTT.c"
        "${NRF_SDK_ROOT}/external/segger_rtt/SEGGER_RTT_printf.c"
    )
    set(nRF5_SDK_11_INCLUDE_DIRS ${nRF5_SDK_11_INCLUDE_DIRS}
        "${NRF_SDK_ROOT}/external/segger_rtt"
    )
    # Set specific toolchain related file
    if(TOOLCHAIN MATCHES "armcc")
        set(nRF5_SDK_11_SOURCE_FILES ${nRF5_SDK_11_SOURCE_FILES}
            "${NRF_SDK_ROOT}/external/segger_rtt/RTT_Syscalls_KEIL.c"
        )
    else()
        set(nRF5_SDK_11_SOURCE_FILES ${nRF5_SDK_11_SOURCE_FILES}
            "${NRF_SDK_ROOT}/external/segger_rtt/RTT_Syscalls_GCC.c"
        )
    endif()
endif()


##############################################################
##############################################################







set(USER_DEFINITIONS
    -DPROJECT_NAME="${PROJECT_NAME}"
    -D${FW_TYPE}
    -D_RTE_
    -DSOFTDEVICE_PRESENT
    -DS130
    -DBLE_STACK_SUPPORT_REQD
    -DSWI_DISABLE0
    -DPB_FIELD_16BIT=1
    -DVERSION_STR="${VERSION_STR}"
    -DSER_CONNECTIVITY                      #serialization采用串口的方式
    -DAPP_SCHEDULER_WITH_PAUSE
)

if(FW_TYPE STREQUAL "DEBUG")
    set(USER_DEFINITIONS ${USER_DEFINITIONS}
        -DTRACE_ENABLE
        -DTRACE_LEVEL=${TRACE_LEVEL}
        -DNRF_LOG_USES_${NRF_LOG_BACKEND}=1
    )
endif()

if(FW_TYPE STREQUAL "FIRMWARE")
    set(USER_DEFINITIONS ${USER_DEFINITIONS}
        -DPROD=1
    )
endif()

# LPWAN region selection
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_AS923=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_AU915=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_AU915_HYBRID=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_CN470=1)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_CN779=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_EU433=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_EU868=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_KR920=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_IN865=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_US915=0)
set(USER_DEFINITIONS ${USER_DEFINITIONS} -DREGION_US915_HYBRID=0)

if(ARMCC_ENABLE_MICROLIB)
    set(USER_DEFINITIONS ${USER_DEFINITIONS}
        -D__MICROLIB)
endif()

add_executable(${target}
    ${APP_SOURCE_FILES}
    ${PRJ_SDK_LPWAN_SOURCE_FILES}
    ${PRJ_SDK_SOURCE_FILES}
    ${nRF5_SDK_11_SOURCE_FILES}
    ${${PLATFORM}_SOURCE_FILES}
    ${SDK_SOURCE_FILES}
)

target_include_directories(${target} PUBLIC
    ${APP_INCLUDE_DIRS}
    ${SDK_INCLUDE_DIRS}
    ${PRJ_SDK_LPWAN_INCLUDE_DIRS}
    ${PRJ_SDK_INCLUDE_DIRS}
    ${nRF5_SDK_11_INCLUDE_DIRS}
    ${${PLATFORM}_INCLUDE_DIRS}
#    ${${BOARD}_INCLUDE_DIRS}
)

set_target_link_options(${target}
    ${CMAKE_SOURCE_DIR}/linker/${PLATFORM})

target_compile_options(${target} PUBLIC
    ${${ARCH}_DEFINES})

target_compile_definitions(${target} PUBLIC
    ${USER_DEFINITIONS}
    ${${PLATFORM}_DEFINES}
    ${${BOARD}_DEFINES}    
)

create_hex(${target})
add_flash_target(${target})

message("USER_DEFINITIONS ========> ${USER_DEFINITIONS}")
#message("factory NRF_LOG_BACKEND: ${NRF_LOG_BACKEND}")