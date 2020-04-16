#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CuTest.h"

#include "ble_handler.h"


void TestBleDataCheck(CuTest* tc)
{
    bool rc = false;

    // test BLE_GAP_AD_TYPE_FLAGS
    rc = ble_data_check((uint8_t *)"\x02\x01", 2);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_FLAGS
    rc = ble_data_check((uint8_t *)"\x02\x01\04", 3);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_FLAGS
    rc = ble_data_check((uint8_t *)"\x02\x01\05", 3);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_FLAGS
    rc = ble_data_check((uint8_t *)"\x02\x01\06", 3);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_FLAGS
    rc = ble_data_check((uint8_t *)"\x02\x01\07", 3);
    CuAssert(tc, "BleDataCheck", false == rc);
        
    // test BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x05\x02\x01\x01\x01\x02", 9);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\x03\x01\x01\x01\x02\x01\x03", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x06\x03\x01\x01\x01\x02\x01", 10);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x01\x03", 5);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x11\x06\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff", 21);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x11\x07\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff", 21);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x10\x07\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee", 20);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x01\x07", 5);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x05\x08\x31\x31\x32\x32", 9);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x05\x09\x31\x31\x32\x32", 9);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_16BIT
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\x14\x01\x01\x01\x02\x01\x03", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_128BIT
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x11\x15\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff", 21);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_SERVICE_DATA
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x11\x16\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff", 21);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_PUBLIC_TARGET_ADDRESS
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\x17\x00\x11\x22\x33\x44\x55", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_RANDOM_TARGET_ADDRESS
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\x18\x00\x11\x22\x33\x44\x55", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_RANDOM_TARGET_ADDRESS
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x08\x18\x00\x11\x22\x33\x44\x55\x66", 12);
    CuAssert(tc, "BleDataCheck", false == rc);
    
    // test BLE_GAP_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\x1b\x00\x11\x22\x33\x44\x55", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
    
    // test BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA
    rc = ble_data_check((uint8_t *)"\x02\x01\x06\x07\xff\x00\x11\x22\x33\x44\x55", 11);
    CuAssert(tc, "BleDataCheck", true == rc);
}



CuSuite* TestBleHandlerCuGetSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, TestBleDataCheck);
 
	return suite;
}

