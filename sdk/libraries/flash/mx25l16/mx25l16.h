/*
 * COPYRIGHT (c) 2010-2014 MACRONIX INTERNATIONAL CO., LTD
 * SPI Flash Low Level Driver (LLD) Sample Code
 *
 * SPI interface command hex code, type definition and function prototype.
 *
 * $Id: MX25_CMD.h,v 1.20 2013/11/08 01:41:48 modelqa Exp $
 */
#ifndef    __MX25_CMD_H__
#define    __MX25_CMD_H__

/* Select your flash device type */
#define MX25L1606E

/* Note:
   Synchronous IO     : MCU will polling WIP bit after
                        sending prgram/erase command
   Non-synchronous IO : MCU can do other operation after
                        sending prgram/erase command
   Default is synchronous IO
*/
//#define    NON_SYNCHRONOUS_IO

/*
  Type and Constant Define
*/

// variable
#define    BYTE_LEN          8
#define    IO_MASK           0x80
#define    HALF_WORD_MASK    0x0000ffff
#define    DUMMY_BYTE        0xFF

/*
  Flash Related Parameter Define
*/

#define    Block_Offset       0x10000   // 64K Block size
#define    Block32K_Offset    0x8000    // 32K Block size
#define    Sector_Offset      0x1000    // 4K Sector size
#define    Page_Offset        0x0100    // 256 Byte Page size
#define    Page32_Offset      0x0020    // 32 Byte Page size (some products have smaller page size)
#define    Block_Num          (FlashSize / Block_Offset)

// Flash control register mask define
// status register
#define    FLASH_WIP_MASK         0x01
#define    FLASH_LDSO_MASK        0x02
#define    FLASH_QE_MASK          0x40
// security register
#define    FLASH_OTPLOCK_MASK     0x03
#define    FLASH_4BYTE_MASK       0x04
#define    FLASH_WPSEL_MASK       0x80
// configuration reigster
#define    FLASH_DC_MASK          0x80
#define    FLASH_DC_2BIT_MASK     0xC0
// other
#define    BLOCK_PROTECT_MASK     0xff
#define    BLOCK_LOCK_MASK        0x01

/* Timer Parameter */
//#define  TIMEOUT    1
//#define  TIMENOTOUT 0
//#define  TPERIOD    240             // ns, The time period of timer count one
//#define  COUNT_ONE_MICROSECOND  16  // us, The loop count value within one micro-second

/*
  System Information Define
*/

//--- insert your MCU information ---//
#define    CLK_PERIOD               63  // unit: ns
#define    Min_Cycle_Per_Inst       12  // cycle count of one instruction
#define    One_Loop_Inst            8   // instruction count of one loop (estimate)

/*
  Flash ID, Timing Information Define
  (The following information could get from device specification)
*/

#ifdef MX25L1606E
#define    FlashID          0xc22015
#define    ElectronicID     0x14
#define    RESID0           0xc214
#define    RESID1           0x14c2
#define    FlashSize        0x200000    // 2 MB
#define    CE_period        10416667    // tCE /  ( CLK_PERIOD * Min_Cycle_Per_Inst *One_Loop_Inst)
#define    tW               40000000    // 40ms
#define    tDP              10000       // 10us
#define    tBP              50000       // 50us
#define    tPP              3000000     // 3ms
#define    tSE              200000000   // 200ms
#define    tBE              2000000000  // 2s
#define    tPUW             10000000    // 10ms
#define    tWSR             tBP
#endif

// Flash information define
#define    WriteStatusRegCycleTime     tW / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#define    PageProgramCycleTime        tPP / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#define    SectorEraseCycleTime        tSE / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#define    BlockEraseCycleTime         tBE / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#define    ChipEraseCycleTime          CE_period
#define    FlashFullAccessTime         tPUW / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)

#ifdef tBP
#define    ByteProgramCycleTime        tBP / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#endif
#ifdef tWSR
#define    WriteSecuRegCycleTime       tWSR / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#endif
#ifdef tBE32
#define    BlockErase32KCycleTime      tBE32 / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#endif
#ifdef tWREAW
#define    WriteExtRegCycleTime        tWREAW / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst)
#endif

/*** MX25 series command hex code definition ***/
//ID comands
#define    FLASH_CMD_RDID      0x9F     //RDID (Read Identification)
#define    FLASH_CMD_RES       0xAB     //RES (Read Electronic ID)
#define    FLASH_CMD_REMS      0x90     //REMS (Read Electronic & Device ID)

//Register comands
#define    FLASH_CMD_WRSR      0x01     //WRSR (Write Status Register)
#define    FLASH_CMD_RDSR      0x05     //RDSR (Read Status Register)
#define    FLASH_CMD_WRSCUR    0x2F     //WRSCUR (Write Security Register)
#define    FLASH_CMD_RDSCUR    0x2B     //RDSCUR (Read Security Register)

//READ comands
#define    FLASH_CMD_READ        0x03   //READ (1 x I/O)
#define    FLASH_CMD_FASTREAD    0x0B   //FAST READ (Fast read data)
#define    FLASH_CMD_DREAD       0x3B   //DREAD (1In/2 Out fast read)
#define    FLASH_CMD_RDSFDP      0x5A   //RDSFDP (Read SFDP)

//Program comands
#define    FLASH_CMD_WREN     0x06      //WREN (Write Enable)
#define    FLASH_CMD_WRDI     0x04      //WRDI (Write Disable)
#define    FLASH_CMD_PP       0x02      //PP (page program)

//Erase comands
#define    FLASH_CMD_SE       0x20      //SE (Sector Erase)
#define    FLASH_CMD_BE       0xD8      //BE (Block Erase)
#define    FLASH_CMD_CE       0x60      //CE (Chip Erase) hex code: 60 or C7

//Mode setting comands
#define    FLASH_CMD_DP       0xB9      //DP (Deep Power Down)
#define    FLASH_CMD_RDP      0xAB      //RDP (Release form Deep Power Down)
#define    FLASH_CMD_ENSO     0xB1      //ENSO (Enter Secured OTP)
#define    FLASH_CMD_EXSO     0xC1      //EXSO  (Exit Secured OTP)
#ifdef SBL_CMD_0x77
#else
#endif

//Reset comands

//Security comands
#ifdef LCR_CMD_0xDD_0xD5
#else
#endif

//Suspend/Resume comands

// Return Message
typedef enum
{
    FlashOperationSuccess,
    FlashWriteRegFailed,
    FlashTimeOut,
    FlashIsBusy,
    FlashQuadNotEnable,
    FlashAddressInvalid
} ReturnMsg;

// Flash status structure define
struct sFlashStatus
{
    /* Mode Register:
     * Bit  Description
     * -------------------------
     *  7   RYBY enable
     *  6   Reserved
     *  5   Reserved
     *  4   Reserved
     *  3   Reserved
     *  2   Reserved
     *  1   Parallel mode enable
     *  0   QPI mode enable
     */
    uint8_t ModeReg;
    bool ArrangeOpt;
};

typedef struct sFlashStatus FlashStatus;

typedef struct mx25_s mx25_t;

struct mx25_s
{
    void (*init) (void);
    void (*deinit) (void);
    void (*chip_select) (void);
    void (*chip_unselect) (void);
     uint8_t(*rw_byte) (uint8_t byte);
};

/* Basic functions */
void mx25flash_init(mx25_t * p_mx25);

/* Utility functions */
bool WaitFlashReady(uint32_t ExpectTime);
bool IsFlashBusy(void);
bool IsFlash4Byte(void);
void SendFlashAddr(uint32_t flash_address, bool addr_4byte_mode);

/* Flash commands */
ReturnMsg mx25_cmd_RDID(uint32_t * Identification);
ReturnMsg mx25_cmd_RES(uint8_t * ElectricIdentification);
ReturnMsg mx25_cmd_REMS(uint16_t * REMS_Identification, FlashStatus * fsptr);

ReturnMsg mx25_cmd_RDSR(uint8_t * StatusReg);
#ifdef SUPPORT_WRSR_CR
ReturnMsg mx25_cmd_WRSR(uint16_t UpdateValue);
#else
ReturnMsg mx25_cmd_WRSR(uint8_t UpdateValue);
#endif
ReturnMsg mx25_cmd_RDSCUR(uint8_t * SecurityReg);
ReturnMsg mx25_cmd_WRSCUR(void);

ReturnMsg mx25_cmd_FASTREAD(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length);
ReturnMsg mx25_cmd_RDSFDP(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length);

ReturnMsg mx25_cmd_WREN(void);
ReturnMsg mx25_cmd_WRDI(void);
ReturnMsg mx25_cmd_PP(uint32_t flash_address, uint8_t * source_address, uint32_t byte_length);

ReturnMsg mx25_cmd_SE(uint32_t flash_address);
ReturnMsg mx25_cmd_BE(uint32_t flash_address);
ReturnMsg mx25_cmd_CE(void);

ReturnMsg mx25_cmd_DP(void);
ReturnMsg mx25_cmd_RDP(void);
ReturnMsg mx25_cmd_ENSO(void);
ReturnMsg mx25_cmd_EXSO(void);

ReturnMsg mx25_flash_read(uint32_t flash_address, uint8_t * target_address, uint32_t byte_length);
ReturnMsg mx25_flash_bufferwrite(uint32_t flash_addr, uint8_t * p_src, uint32_t data_len);

#endif                          /* __MX25_CMD_H__ */
