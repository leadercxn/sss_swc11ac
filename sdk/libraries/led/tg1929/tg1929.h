/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __TG1929_H__
#define __TG1929_H__

// Device address
#define TG1929_DEVICE_ADDRESS0  (0x4E)  // ADDR-VDD
#define TG1929_DEVICE_ADDRESS1  (0x46)  // ADDR-GND
#define TG1929_DEVICE_ADDRESS2  (0x4A)  // ADDR-SCL
#define TG1929_DEVICE_ADDRESS3  (0x4C)  // ADDR-SDA

// Registers
#define	TG1929_REG_RSTCTR 	    	(0x00)  // 复位寄存器
#define	TG1929_REG_RGB_OE 	       	(0x01)  // LED 使能寄存器
#define	TG1929_REG_FIXBRIT_LED0 	(0x02)  // LED0 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED1 	(0x03)  // LED1 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED2 	(0x04)  // LED2 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED3 	(0x05)  // LED3 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED4 	(0x06)  // LED4 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED5 	(0x07)  // LED5 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED6 	(0x08)  // LED6 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED7 	(0x09)  // LED7 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED8 	(0x0A)  // LED8 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED9 	(0x0B)  // LED9 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED10 	(0x0C)  // LED10 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED11 	(0x0D)  // LED11 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED12   	(0x0E)  // LED12 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED13 	(0x0F)  // LED13 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED14 	(0x10)  // LED14 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED15 	(0x11)  // LED15 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED16 	(0x12)  // LED16 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_LED17 	(0x13)  // LED17 亮度调节寄存器
#define	TG1929_REG_FIXBRIT_MAX_NUM	(0x14)

// Const value
#define TG1929_FIXBRIT_MAX      (0x7F)

#define TG1929_POWER_ON			(0x00)
#define TG1929_POWER_OFF		(0x01)

#define TG1929_RGB0_DISABLE		(0 << 0)
#define TG1929_RGB1_ENABLE		(1 << 1)
#define TG1929_RGB1_DISABLE		(0 << 1)
#define TG1929_RGB2_ENABLE		(1 << 2)
#define TG1929_RGB2_DISABLE		(0 << 2)
#define TG1929_RGB3_ENABLE		(1 << 3)
#define TG1929_RGB3_DISABLE		(0 << 3)
#define TG1929_RGB4_ENABLE		(1 << 4)
#define TG1929_RGB4_DISABLE		(0 << 4)
#define TG1929_RGB5_ENABLE		(1 << 5)
#define TG1929_RGB5_DISABLE		(0 << 5)

#define TG1929_RGBALL_ENABLE	(0x3F)
#define TG1929_RGBALL_DISABLE	(0x00)

#define TG1929_RGB_ENABLE		(0)
#define TG1929_RGB_DISABLE		(1)

#define TG1929_RGB0				(0)
#define TG1929_RGB1				(1)
#define TG1929_RGB2				(2)
#define TG1929_RGB3				(3)
#define TG1929_RGB4				(4)
#define TG1929_RGB5				(5)
#define TG1929_RGB_MAX			(6)

typedef struct tg1929_s tg1929_t;

struct tg1929_s
{
    uint8_t address;
     bool(*reset) (tg1929_t * p_tg1929);
     bool(*write) (tg1929_t * p_tg1929, uint8_t addr, uint8_t * p_data, uint16_t size);
};

bool tg1929_reg_write(tg1929_t * p_tg1929, uint8_t addr, uint8_t value);

/**
 * [tg1929_sw_reset description]
 *
 * @param     p_tg1929    [description]
 *
 * @return                [description]
 */
bool tg1929_sw_reset(tg1929_t * p_tg1929);

/**
 * [tg1929_rgb_oe_set description]
 *
 * @param     p_tg1929    [description]
 * @param     value       [description]
 *
 * @return                [description]
 */
bool tg1929_rgb_oe_set(tg1929_t * p_tg1929, uint8_t value);

/**
 * [tg1929_fixbrit_set description]
 *
 * @param     p_tg1929    [description]
 * @param     led         [description]
 * @param     value       [description]
 *
 * @return                [description]
 */
bool tg1929_fixbrit_set(tg1929_t * p_tg1929, uint8_t led, uint8_t value);

/**
 * [tg1929_rgb_set description]
 *
 * @param     p_tg1929    [description]
 * @param     rgb_led     [description]
 * @param     r           [description]
 * @param     g           [description]
 * @param     b           [description]
 *
 * @return                [description]
 */
bool tg1929_rgb_set(tg1929_t * p_tg1929, uint8_t rgb_led, uint8_t r, uint8_t g, uint8_t b);

#endif
