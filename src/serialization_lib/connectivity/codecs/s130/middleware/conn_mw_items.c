#include "conn_mw_nrf_soc.h"
#include "conn_mw_ble.h"
#include "conn_mw_ble_l2cap.h"
#include "conn_mw_ble_gap.h"
#include "conn_mw_ble_gatts.h"
#include "conn_mw_ble_gattc.h"
#include "conn_mw.h"

#include "nrf_soc.h"
#include "ble.h"
#include "ser_alpha.h"
#include "ser_conn_alpha_event_encorder.h"

/**@brief Connectivity middleware handlers table. */
static const conn_mw_item_t conn_mw_item[] = {
    //Functions from nrf_soc.h
    {SD_POWER_SYSTEM_OFF, conn_mw_power_system_off},                                        // 60
    {SD_TEMP_GET, conn_mw_temp_get},                                                        // 83
    {SD_ECB_BLOCK_ENCRYPT, conn_mw_ecb_block_encrypt},                                      // 77
    //Functions from ble.h          
    {SD_BLE_TX_PACKET_COUNT_GET, conn_mw_ble_tx_packet_count_get},                          // 98
    {SD_BLE_UUID_VS_ADD, conn_mw_ble_uuid_vs_add},                                          // 99
    {SD_BLE_UUID_DECODE, conn_mw_ble_uuid_decode},                                          // 100
    {SD_BLE_UUID_ENCODE, conn_mw_ble_uuid_encode},                                          // 101
    {SD_BLE_VERSION_GET, conn_mw_ble_version_get},                                          // 102
    {SD_BLE_OPT_GET, conn_mw_ble_opt_get},                                                  // 105
    {SD_BLE_OPT_SET, conn_mw_ble_opt_set},                                                  // 104
    {SD_BLE_ENABLE, conn_mw_ble_enable},                                                    // 96
    {SD_BLE_USER_MEM_REPLY, conn_mw_ble_user_mem_reply},                                    // 103
    //Functions from ble_l2cap.h            
    {SD_BLE_L2CAP_CID_REGISTER, conn_mw_ble_l2cap_cid_register},                            // 176
    {SD_BLE_L2CAP_CID_UNREGISTER, conn_mw_ble_l2cap_cid_unregister},                        // 177
    {SD_BLE_L2CAP_TX, conn_mw_ble_l2cap_tx},                                                // 178
    //Functions from ble_gap.h          
    {SD_BLE_GAP_SCAN_STOP, conn_mw_ble_gap_scan_stop},                                      // 139
    {SD_BLE_GAP_ADDRESS_SET, conn_mw_ble_gap_address_set},                                  // 112
    {SD_BLE_GAP_CONNECT, conn_mw_ble_gap_connect},                                          // 140
    {SD_BLE_GAP_CONNECT_CANCEL, conn_mw_ble_gap_connect_cancel},                            // 141
    {SD_BLE_GAP_SCAN_START, conn_mw_ble_gap_scan_start},                                    // 138
    {SD_BLE_GAP_SEC_INFO_REPLY, conn_mw_ble_gap_sec_info_reply},                            // 134
    {SD_BLE_GAP_ENCRYPT, conn_mw_ble_gap_encrypt},                                          // 133
    {SD_BLE_GAP_ADDRESS_GET, conn_mw_ble_gap_address_get},                                  // 113
    {SD_BLE_GAP_ADV_DATA_SET, conn_mw_ble_gap_adv_data_set},                                // 114
    {SD_BLE_GAP_ADV_START, conn_mw_ble_gap_adv_start},                                      // 115
    {SD_BLE_GAP_ADV_STOP, conn_mw_ble_gap_adv_stop},                                        // 116
    {SD_BLE_GAP_CONN_PARAM_UPDATE, conn_mw_ble_gap_conn_param_update},                      // 117
    {SD_BLE_GAP_DISCONNECT, conn_mw_ble_gap_disconnect},                                    // 118
    {SD_BLE_GAP_TX_POWER_SET, conn_mw_ble_gap_tx_power_set},                                // 119
    {SD_BLE_GAP_APPEARANCE_SET, conn_mw_ble_gap_appearance_set},                            // 120
    {SD_BLE_GAP_APPEARANCE_GET, conn_mw_ble_gap_appearance_get},                            // 121
    {SD_BLE_GAP_PPCP_SET, conn_mw_ble_gap_ppcp_set},                                        // 122
    {SD_BLE_GAP_PPCP_GET, conn_mw_ble_gap_ppcp_get},                                        // 123
    {SD_BLE_GAP_DEVICE_NAME_SET, conn_mw_ble_gap_device_name_set},                          // 124
    {SD_BLE_GAP_DEVICE_NAME_GET, conn_mw_ble_gap_device_name_get},                          // 125
    {SD_BLE_GAP_AUTHENTICATE, conn_mw_ble_gap_authenticate},                                // 126
    {SD_BLE_GAP_SEC_PARAMS_REPLY, conn_mw_ble_gap_sec_params_reply},                        // 127
    {SD_BLE_GAP_AUTH_KEY_REPLY, conn_mw_ble_gap_auth_key_reply},                            // 128
    {SD_BLE_GAP_SEC_INFO_REPLY, conn_mw_ble_gap_sec_info_reply},                            // 134
    {SD_BLE_GAP_CONN_SEC_GET, conn_mw_ble_gap_conn_sec_get},                                // 135
    {SD_BLE_GAP_RSSI_START, conn_mw_ble_gap_rssi_start},                                    // 136
    {SD_BLE_GAP_RSSI_STOP, conn_mw_ble_gap_rssi_stop},                                      // 137
    {SD_BLE_GAP_KEYPRESS_NOTIFY, conn_mw_ble_gap_keypress_notify},                          // 130
    {SD_BLE_GAP_LESC_DHKEY_REPLY, conn_mw_ble_gap_lesc_dhkey_reply},                        // 129
    {SD_BLE_GAP_LESC_OOB_DATA_SET, conn_mw_ble_gap_lesc_oob_data_set},                      // 132
    {SD_BLE_GAP_LESC_OOB_DATA_GET, conn_mw_ble_gap_lesc_oob_data_get},                      // 131
    //Functions from ble_gattc.h
    {SD_BLE_GATTC_PRIMARY_SERVICES_DISCOVER, conn_mw_ble_gattc_primary_services_discover},  // 144
    {SD_BLE_GATTC_RELATIONSHIPS_DISCOVER, conn_mw_ble_gattc_relationships_discover},        // 145
    {SD_BLE_GATTC_CHARACTERISTICS_DISCOVER, conn_mw_ble_gattc_characteristics_discover},    // 146
    {SD_BLE_GATTC_DESCRIPTORS_DISCOVER, conn_mw_ble_gattc_descriptors_discover},            // 147
    {SD_BLE_GATTC_CHAR_VALUE_BY_UUID_READ, conn_mw_ble_gattc_char_value_by_uuid_read},      // 149
    {SD_BLE_GATTC_READ, conn_mw_ble_gattc_read},                                            // 150
    {SD_BLE_GATTC_CHAR_VALUES_READ, conn_mw_ble_gattc_char_values_read},                    // 151
    {SD_BLE_GATTC_WRITE, conn_mw_ble_gattc_write},                                          // 152
    {SD_BLE_GATTC_HV_CONFIRM, conn_mw_ble_gattc_hv_confirm},                                // 153
    {SD_BLE_GATTC_ATTR_INFO_DISCOVER, conn_mw_ble_gattc_attr_info_discover},                // 148
    //Functions from ble_gatts.h
    {SD_BLE_GATTS_SERVICE_ADD, conn_mw_ble_gatts_service_add},                              // 160
    {SD_BLE_GATTS_INCLUDE_ADD, conn_mw_ble_gatts_include_add},                              // 161
    {SD_BLE_GATTS_CHARACTERISTIC_ADD, conn_mw_ble_gatts_characteristic_add},                // 162
    {SD_BLE_GATTS_DESCRIPTOR_ADD, conn_mw_ble_gatts_descriptor_add},                        // 163
    {SD_BLE_GATTS_VALUE_SET, conn_mw_ble_gatts_value_set},                                  // 164
    {SD_BLE_GATTS_VALUE_GET, conn_mw_ble_gatts_value_get},                                  // 165
    {SD_BLE_GATTS_HVX, conn_mw_ble_gatts_hvx},                                              // 166
    {SD_BLE_GATTS_SERVICE_CHANGED, conn_mw_ble_gatts_service_changed},                      // 167
    {SD_BLE_GATTS_RW_AUTHORIZE_REPLY, conn_mw_ble_gatts_rw_authorize_reply},                // 168
    {SD_BLE_GATTS_SYS_ATTR_SET, conn_mw_ble_gatts_sys_attr_set},                            // 169
    {SD_BLE_GATTS_SYS_ATTR_GET, conn_mw_ble_gatts_sys_attr_get},                            // 170
    //Functions form alpha
    {ALPHA_ALIVE_ACCESS, conn_mw_alpha_alive_access},                                       // 1


};
