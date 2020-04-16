/*
 * COPYRIGHT (c) 2010-2014 MACRONIX INTERNATIONAL CO., LTD
 * SPI Flash Low Level Driver (LLD) Sample Code
 *
 * SPI interface command set
 *
 * $Id: MX25_CMD.c,v 1.29 2013/08/12 02:56:37 mxclldb1 Exp $
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "mx25l16.h"

static mx25_t m_mx25;

/*
 --Local variables
 */

/*
 --Common functions
 */

/*
 * Function:       mx25_chip_select
 * Description:    chip select
 */
void mx25_chip_select(void)
{
    m_mx25.chip_select();
}

/*
 * Function:       mx25_chip_unselect
 * Description:    chip unselect
 */
void mx25_chip_unselect(void)
{
    m_mx25.chip_unselect();
}

/*
 * Function:       mx25_rw_byte
 * Arguments:      @byte, data you want to pass
 * Description:    Write a byte or dummy data into SPI bus.
 * Return Message: Data on MISO.
 */
uint8_t mx25_rw_byte(uint8_t byte)
{
    return m_mx25.rw_byte(byte);
}

/*
 * Function:       mx25flash_init
 * Arguments:      None
 * Description:    Initial spi flash state and wait flash warm-up
 *                 (enable read/write).
 * Return Message: None
 */
void mx25flash_init(mx25_t * p_mx25)
{
    m_mx25.init = p_mx25->init;
    m_mx25.deinit = p_mx25->deinit;
    m_mx25.chip_select = p_mx25->chip_select;
    m_mx25.chip_unselect = p_mx25->chip_unselect;
    m_mx25.rw_byte = p_mx25->rw_byte;

    m_mx25.init();
}

/*
 * Function:       WaitFlashReady
 * Arguments:      ExpectTime, expected time-out value of flash operations.
 *                 No use at non-synchronous IO mode.
 * Description:    Synchronous IO:
 *                 If flash is ready return true.
 *                 If flash is time-out return false.
 *                 Non-synchronous IO:
 *                 Always return true
 * Return Message: true, false
 */
bool WaitFlashReady(uint32_t ExpectTime)
{
#ifndef NON_SYNCHRONOUS_IO
    uint32_t temp = 0;
    while (IsFlashBusy())
    {
        if (temp > ExpectTime)
        {
            return false;
        }
        temp = temp + 1;
    }
    return true;
#else
    return true;
#endif
}

/*
 * Function:       IsFlashBusy
 * Arguments:      None.
 * Description:    Check status register WIP bit.
 *                 If  WIP bit = 1: return true ( Busy )
 *                             = 0: return false ( Ready ).
 * Return Message: true, false
 */
bool IsFlashBusy(void)
{
    uint8_t gDataBuffer;

    mx25_cmd_RDSR(&gDataBuffer);
    if ((gDataBuffer & FLASH_WIP_MASK) == FLASH_WIP_MASK)
        return true;
    else
        return false;
}

/*
 * Function:       IsFlash4Byte
 * Arguments:      None
 * Description:    Check flash address is 3-byte or 4-byte.
 *                 If flash 4BYTE bit = 1: return true
 *                                    = 0: return false.
 * Return Message: true, false
 */
bool IsFlash4Byte(void)
{
#if 0
#ifdef FLASH_CMD_RDSCUR
#ifdef FLASH_4BYTE_ONLY
    return true;
#elif FLASH_3BYTE_ONLY
    return false;
#else
    uint8_t gDataBuffer;
    mx25_cmd_RDSCUR(&gDataBuffer);
    if ((gDataBuffer & FLASH_4BYTE_MASK) == FLASH_4BYTE_MASK)
        return true;
    else
        return false;
#endif
#else
    return false;
#endif
#endif
    return false;
}

/*
 * Function:       SendFlashAddr
 * Arguments:      flash_address, 32 bit flash memory address
 *                 io_mode, I/O mode to transfer address
 *                 addr_4byte_mode,
 * Description:    Send flash address with 3-byte or 4-byte mode.
 * Return Message: None
 */
void SendFlashAddr(uint32_t flash_address, bool addr_4byte_mode)
{
    /* Check flash is 3-byte or 4-byte mode.
       4-byte mode: Send 4-byte address (A31-A0)
       3-byte mode: Send 3-byte address (A23-A0) */
    if (addr_4byte_mode == true)
    {
        mx25_rw_byte((flash_address >> 24));    // A31-A24
    }
    /* A23-A0 */
    mx25_rw_byte((flash_address >> 16));
    mx25_rw_byte((flash_address >> 8));
    mx25_rw_byte((flash_address));
}

/*
 * ID Command
 */

/*
 * Function:       CMD_RDID
 * Arguments:      Identification, 32 bit buffer to store id
 * Description:    The RDID instruction is to read the manufacturer ID
 *                 of 1-byte and followed by Device ID of 2-byte.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RDID(uint32_t * Identification)
{
    uint32_t temp;
    uint8_t gDataBuffer[3];

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Send command
    mx25_rw_byte(FLASH_CMD_RDID);

    // Get manufacturer identification, device identification
    gDataBuffer[0] = mx25_rw_byte(DUMMY_BYTE);
    gDataBuffer[1] = mx25_rw_byte(DUMMY_BYTE);
    gDataBuffer[2] = mx25_rw_byte(DUMMY_BYTE);

    // Chip select go high to end a command
    mx25_chip_unselect();

    // Store identification
    temp = gDataBuffer[0];
    temp = (temp << 8) | gDataBuffer[1];
    *Identification = (temp << 8) | gDataBuffer[2];

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_RES
 * Arguments:      ElectricIdentification, 8 bit buffer to store electric id
 * Description:    The RES instruction is to read the Device
 *                 electric identification of 1-byte.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RES(uint8_t * ElectricIdentification)
{

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Send flash command and insert dummy cycle
    mx25_rw_byte(FLASH_CMD_RES);
    mx25_rw_byte(DUMMY_BYTE);
    mx25_rw_byte(DUMMY_BYTE);
    mx25_rw_byte(DUMMY_BYTE);

    // Get electric identification
    *ElectricIdentification = mx25_rw_byte(DUMMY_BYTE);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_REMS
 * Arguments:      REMS_Identification, 16 bit buffer to store id
 *                 fsptr, pointer of flash status structure
 * Description:    The REMS instruction is to read the Device
 *                 manufacturer ID and electric ID of 1-byte.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_REMS(uint16_t * REMS_Identification, FlashStatus * fsptr)
{
    uint8_t gDataBuffer[2];

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Send flash command and insert dummy cycle ( if need )
    // ArrangeOpt = 0x00 will output the manufacturer's ID first
    //            = 0x01 will output electric ID first
    mx25_rw_byte(FLASH_CMD_REMS);
    mx25_rw_byte(DUMMY_BYTE);
    mx25_rw_byte(DUMMY_BYTE);
    mx25_rw_byte(fsptr->ArrangeOpt);

    // Get ID
    gDataBuffer[0] = mx25_rw_byte(DUMMY_BYTE);
    gDataBuffer[1] = mx25_rw_byte(DUMMY_BYTE);

    // Store identification informaion
    *REMS_Identification = (gDataBuffer[0] << 8) | gDataBuffer[1];

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Register  Command
 */

/*
 * Function:       CMD_RDSR
 * Arguments:      StatusReg, 8 bit buffer to store status register value
 * Description:    The RDSR instruction is for reading Status Register Bits.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RDSR(uint8_t * StatusReg)
{
    uint8_t gDataBuffer;

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Send command
    mx25_rw_byte(FLASH_CMD_RDSR);
    gDataBuffer = mx25_rw_byte(DUMMY_BYTE);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    *StatusReg = gDataBuffer;

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_WRSR
 * Arguments:      UpdateValue, 8/16 bit status register value to updata
 * Description:    The WRSR instruction is for changing the values of
 *                 Status Register Bits (and configuration register)
 * Return Message: FlashIsBusy, FlashTimeOut, FlashOperationSuccess
 */
#ifdef SUPPORT_WRSR_CR
ReturnMsg mx25_cmd_WRSR(uint16_t UpdateValue)
#else
ReturnMsg mx25_cmd_WRSR(uint8_t UpdateValue)
#endif
{
    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Send command and update value
    mx25_rw_byte(FLASH_CMD_WRSR);
    mx25_rw_byte(UpdateValue);
#ifdef SUPPORT_WRSR_CR
    mx25_rw_byte(UpdateValue >> 8);     // write configuration register
#endif

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(WriteStatusRegCycleTime))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;

}

/*
 * Function:       CMD_RDSCUR
 * Arguments:      SecurityReg, 8 bit buffer to store security register value
 * Description:    The RDSCUR instruction is for reading the value of
 *                 Security Register bits.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RDSCUR(uint8_t * SecurityReg)
{
    uint8_t gDataBuffer;

    // Chip select go low to start a flash command
    mx25_chip_select();

    //Send command
    mx25_rw_byte(FLASH_CMD_RDSCUR);
    gDataBuffer = mx25_rw_byte(DUMMY_BYTE);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    *SecurityReg = gDataBuffer;

    return FlashOperationSuccess;

}

/*
 * Function:       CMD_WRSCUR
 * Arguments:      None.
 * Description:    The WRSCUR instruction is for changing the values of
 *                 Security Register Bits.
 * Return Message: FlashIsBusy, FlashOperationSuccess, FlashWriteRegFailed,
 *                 FlashTimeOut
 */
ReturnMsg mx25_cmd_WRSCUR(void)
{
    uint8_t gDataBuffer;

    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write WRSCUR command
    mx25_rw_byte(FLASH_CMD_WRSCUR);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(WriteSecuRegCycleTime))
    {

        mx25_cmd_RDSCUR(&gDataBuffer);

        // Check security register LDSO bit
        if ((gDataBuffer & FLASH_LDSO_MASK) == FLASH_LDSO_MASK)
            return FlashOperationSuccess;
        else
            return FlashWriteRegFailed;
    }
    else
        return FlashTimeOut;

}

/*
 * Read Command
 */

/*
 * Function:       CMD_FASTREAD
 * Arguments:      flash_address, 32 bit flash memory address
 *                 target_address, buffer address to store returned data
 *                 byte_length, length of returned data in byte unit
 * Description:    The FASTREAD instruction is for quickly reading data out.
 * Return Message: FlashAddressInvalid, FlashOperationSuccess
 */
ReturnMsg mx25_cmd_FASTREAD(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length)
{
    uint32_t index;
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write Fast Read command, address and dummy cycle
    mx25_rw_byte(FLASH_CMD_FASTREAD);
    SendFlashAddr(flash_address, addr_4byte_mode);
    mx25_rw_byte(DUMMY_BYTE);   // Wait dummy cycle

    // Set a loop to read data into data buffer
    for (index = 0; index < byte_length; index++)
    {
        *(target_address + index) = mx25_rw_byte(DUMMY_BYTE);
    }

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_RDSFDP
 * Arguments:      flash_address, 32 bit flash memory address
 *                 target_address, buffer address to store returned data
 *                 byte_length, length of returned data in byte unit
 * Description:    RDSFDP can retrieve the operating characteristics, structure
 *                 and vendor-specified information such as identifying information,
 *                 memory size, operating voltages and timinginformation of device
 * Return Message: FlashAddressInvalid, FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RDSFDP(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length)
{
    uint32_t index;
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write Read SFDP command
    mx25_rw_byte(FLASH_CMD_RDSFDP);
    SendFlashAddr(flash_address, addr_4byte_mode);
    mx25_rw_byte(DUMMY_BYTE);   // Insert dummy cycle

    // Set a loop to read data into data buffer
    for (index = 0; index < byte_length; index++)
    {
        *(target_address + index) = mx25_rw_byte(DUMMY_BYTE);
    }

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Program Command
 */

/*
 * Function:       CMD_WREN
 * Arguments:      None.
 * Description:    The WREN instruction is for setting
 *                 Write Enable Latch (WEL) bit.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_WREN(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write Enable command = 0x06, Setting Write Enable Latch Bit
    mx25_rw_byte(FLASH_CMD_WREN);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_WRDI
 * Arguments:      None.
 * Description:    The WRDI instruction is to reset
 *                 Write Enable Latch (WEL) bit.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_WRDI(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write Disable command = 0x04, resets Write Enable Latch Bit
    mx25_rw_byte(FLASH_CMD_WRDI);

    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_PP
 * Arguments:      flash_address, 32 bit flash memory address
 *                 source_address, buffer address of source data to program
 *                 byte_length, byte length of data to programm
 * Description:    The PP instruction is for programming
 *                 the memory to be "0".
 *                 The device only accept the last 256 byte ( or 32 byte ) to program.
 *                 If the page address ( flash_address[7:0] ) reach 0xFF, it will
 *                 program next at 0x00 of the same page.
 *                 Some products have smaller page size ( 32 byte )
 * Return Message: FlashAddressInvalid, FlashIsBusy, FlashOperationSuccess,
 *                 FlashTimeOut
 */
ReturnMsg mx25_cmd_PP(uint32_t flash_address, uint8_t * source_address, uint32_t byte_length)
{
    uint32_t index;
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write Page Program command
    mx25_rw_byte(FLASH_CMD_PP);
    SendFlashAddr(flash_address, addr_4byte_mode);

    // Set a loop to down load whole page data into flash's buffer
    // Note: only last 256 byte ( or 32 byte ) will be programmed
    for (index = 0; index < byte_length; index++)
    {
        mx25_rw_byte(*(source_address + index));
    }

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(PageProgramCycleTime))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}

/*
 * Erase Command
 */

/*
 * Function:       CMD_SE
 * Arguments:      flash_address, 32 bit flash memory address
 * Description:    The SE instruction is for erasing the data
 *                 of the chosen sector (4KB) to be "1".
 * Return Message: FlashAddressInvalid, FlashIsBusy, FlashOperationSuccess,
 *                 FlashTimeOut
 */
ReturnMsg mx25_cmd_SE(uint32_t flash_address)
{
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    //Write Sector Erase command = 0x20;
    mx25_rw_byte(FLASH_CMD_SE);
    SendFlashAddr(flash_address, addr_4byte_mode);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(SectorEraseCycleTime))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}

/*
 * Function:       CMD_BE
 * Arguments:      flash_address, 32 bit flash memory address
 * Description:    The BE instruction is for erasing the data
 *                 of the chosen sector (64KB) to be "1".
 * Return Message: FlashAddressInvalid, FlashIsBusy, FlashOperationSuccess,
 *                 FlashTimeOut
 */
ReturnMsg mx25_cmd_BE(uint32_t flash_address)
{
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    //Write Block Erase command = 0xD8;
    mx25_rw_byte(FLASH_CMD_BE);
    SendFlashAddr(flash_address, addr_4byte_mode);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(BlockEraseCycleTime))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}

/*
 * Function:       CMD_CE
 * Arguments:      None.
 * Description:    The CE instruction is for erasing the data
 *                 of the whole chip to be "1".
 * Return Message: FlashIsBusy, FlashOperationSuccess, FlashTimeOut
 */
ReturnMsg mx25_cmd_CE(void)
{
    // Check flash is busy or not
    if (IsFlashBusy())
        return FlashIsBusy;

    // Setting Write Enable Latch bit
    mx25_cmd_WREN();

    // Chip select go low to start a flash command
    mx25_chip_select();

    //Write Chip Erase command = 0x60;
    mx25_rw_byte(FLASH_CMD_CE);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    if (WaitFlashReady(ChipEraseCycleTime))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}

/*
 * Mode setting Command
 */

/*
 * Function:       CMD_DP
 * Arguments:      None.
 * Description:    The DP instruction is for setting the
 *                 device on the minimizing the power consumption.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_DP(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Deep Power Down Mode command
    mx25_rw_byte(FLASH_CMD_DP);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_RDP
 * Arguments:      None.
 * Description:    The Release from RDP instruction is
 *                 putting the device in the Stand-by Power mode.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_RDP(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Deep Power Down Mode command
    mx25_rw_byte(FLASH_CMD_RDP);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_ENSO
 * Arguments:      None.
 * Description:    The ENSO instruction is for entering the secured OTP mode.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_ENSO(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write ENSO command
    mx25_rw_byte(FLASH_CMD_ENSO);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Function:       CMD_EXSO
 * Arguments:      None.
 * Description:    The EXSO instruction is for exiting the secured OTP mode.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg mx25_cmd_EXSO(void)
{
    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write EXSO command = 0xC1
    mx25_rw_byte(FLASH_CMD_EXSO);

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Read operation
 */

/*
 * Function:       mx25_flash_read
 * Arguments:      @flash_address, 32 bit flash memory address
 *                 @target_address, buffer address to store returned data
 *                 @byte_length, length of returned data in byte unit
 * Description:    The READ instruction is for reading data out.
 * Return Message: FlashAddressInvalid, FlashOperationSuccess
 */
ReturnMsg mx25_flash_read(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length)
{
    uint32_t index;
    uint8_t addr_4byte_mode;

    // Check flash address
    if (flash_address > FlashSize)
        return FlashAddressInvalid;

    // Check 3-byte or 4-byte mode
    if (IsFlash4Byte())
        addr_4byte_mode = true; // 4-byte mode
    else
        addr_4byte_mode = false;        // 3-byte mode

    // Chip select go low to start a flash command
    mx25_chip_select();

    // Write READ command and address
    mx25_rw_byte(FLASH_CMD_READ);
    SendFlashAddr(flash_address, addr_4byte_mode);

    // Set a loop to read data into buffer
    for (index = 0; index < byte_length; index++)
    {
        // Read data one byte at a time
        *(target_address + index) = mx25_rw_byte(DUMMY_BYTE);
    }

    // Chip select go high to end a flash command
    mx25_chip_unselect();

    return FlashOperationSuccess;
}

/*
 * Write operation
 */

/*
 * Function:       mx25_flash_bufferwrite
 * Arguments:      @flash_addr, 32 bit flash memory address
 *                 @p_src, source stores the writing data
 *                 @data_len, length of writing data in byte unit
 * Description:    The WRITE instruction is for writing data in flash.
 * Return Message: FlashAddressInvalid, FlashOperationSuccess
 */
ReturnMsg mx25_flash_bufferwrite(uint32_t flash_addr, uint8_t * p_src, uint32_t data_len)
{
    uint32_t num_page = 0;
    uint32_t num_single = 0;
    uint32_t addr = 0;
    uint32_t count = 0;
    uint32_t temp = 0;
    ReturnMsg flashStatus;

    // Calculate whether the writing flash address is multiple of flash page size
    addr = flash_addr % Page_Offset;
    // Remain 'count' bytes aligning to page address
    count = Page_Offset - addr;
    // How many pages need to write
    num_page = data_len / Page_Offset;
    // Bytes are not full in one page
    num_single = data_len % Page_Offset;

    // If aligned
    if (addr == 0)
    {
        // Data length < Page size
        if (num_page == 0)
        {
            flashStatus = mx25_cmd_PP(flash_addr, p_src, data_len);
            if (flashStatus != FlashOperationSuccess)
                return flashStatus;
        }
        // Data length > Page size
        else
        {
            while (num_page--)
            {
                flashStatus = mx25_cmd_PP(flash_addr, p_src, Page_Offset);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;
                flash_addr += Page_Offset;
                p_src += Page_Offset;
            }
            // Write the remained data
            flashStatus = mx25_cmd_PP(flash_addr, p_src, num_single);
            if (flashStatus != FlashOperationSuccess)
                return flashStatus;
        }
    }
    // If not aligned
    else
    {
        // Data length < Page size
        if (num_page == 0)
        {
            // Remaining data slot in current page is less than the
            // writing data which is not one-page size
            if (num_single > count)
            {
                // Fill the current page first
                temp = num_single - count;
                flashStatus = mx25_cmd_PP(flash_addr, p_src, count);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;

                flash_addr += count;
                p_src += count;

                flashStatus = mx25_cmd_PP(flash_addr, p_src, temp);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;
            }
            else
            {
                flashStatus = mx25_cmd_PP(flash_addr, p_src, data_len);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;
            }
        }
        // Data length > Page size
        else
        {
            data_len -= count;
            num_page = data_len / Page_Offset;
            num_single = data_len % Page_Offset;

            flashStatus = mx25_cmd_PP(flash_addr, p_src, count);
            if (flashStatus != FlashOperationSuccess)
                return flashStatus;

            flash_addr += count;
            p_src += count;

            while (num_page--)
            {
                flashStatus = mx25_cmd_PP(flash_addr, p_src, Page_Offset);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;
                flash_addr += Page_Offset;
                p_src += Page_Offset;
            }
            if (num_single != 0)
            {
                flashStatus = mx25_cmd_PP(flash_addr, p_src, num_single);
                if (flashStatus != FlashOperationSuccess)
                    return flashStatus;
            }
        }
    }
    return FlashOperationSuccess;
}
