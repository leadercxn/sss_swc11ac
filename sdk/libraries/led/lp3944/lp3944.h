/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __LP3944_H__
#define __LP3944_H__

/* 7bit address */
#define LP3944_ADDR0          0x60
#define LP3944_ADDR1          0x61
#define LP3944_ADDR2          0x62
#define LP3944_ADDR3          0x63
#define LP3944_ADDR4          0x64
#define LP3944_ADDR5          0x65
#define LP3944_ADDR6          0x66
#define LP3944_ADDR7          0x67

/* Read Only Registers */
#define LP3944_REG_INPUT1     0x00 /* LEDs 0-7 InputRegister (Read Only) */
#define LP3944_REG_REGISTER1  0x01 /* None (Read Only) */

#define LP3944_REG_PSC0       0x02 /* Frequency Prescaler 0 (R/W) */
#define LP3944_REG_PWM0       0x03 /* PWM Register 0 (R/W) */
#define LP3944_REG_PSC1       0x04 /* Frequency Prescaler 1 (R/W) */
#define LP3944_REG_PWM1       0x05 /* PWM Register 1 (R/W) */
#define LP3944_REG_LS0        0x06 /* LEDs 0-3 Selector (R/W) */
#define LP3944_REG_LS1        0x07 /* LEDs 4-7 Selector (R/W) */

/* These registers are not used to control leds in LP3944, they can store
 * arbitrary values which the chip will ignore.
 */
#define LP3944_REG_REGISTER8  0x08
#define LP3944_REG_REGISTER9  0x09

#define LP3944_DIM0 0
#define LP3944_DIM1 1

/* period in ms */
#define LP3944_PERIOD_MIN 0
#define LP3944_PERIOD_MAX 1600

/* duty cycle is a percentage */
#define LP3944_DUTY_CYCLE_MIN 0
#define LP3944_DUTY_CYCLE_MAX 100

#define LP3944_LED_STATUS_OFF   0x00
#define LP3944_LED_STATUS_ON    0x01
#define LP3944_LED_STATUS_DIM0  0x02
#define LP3944_LED_STATUS_DIM1  0x03
#define LP3944_LED_STATUS_MASK	0x03

#define LP3944_LED0 0
#define LP3944_LED1 1
#define LP3944_LED2 2
#define LP3944_LED3 3
#define LP3944_LED4 4
#define LP3944_LED5 5
#define LP3944_LED6 6
#define LP3944_LED7 7
#define LP3944_LEDS_MAX 8

typedef struct lp3944_s lp3944_t;

struct lp3944_s
{
    uint8_t address;
    bool (*reg_read)(lp3944_t * p_tg1929, uint8_t addr, uint8_t * p_value);
    bool (*reg_write)(lp3944_t * p_tg1929, uint8_t addr, uint8_t p_value);
};

/**
 * [lp3944_led_set description]
 *
 * @param     p_lp3944    [description]
 * @param     led         [description]
 * @param     status      [description]
 *
 * @return                [description]
 */
bool lp3944_led_set(lp3944_t *p_lp3944, uint8_t led, uint8_t status);

/**
 * [lp3944_dim_set_period description]
 *
 * @param     p_lp3944    [description]
 * @param     dim         [description]
 * @param     period      [description]
 *
 * @return                [description]
 */
bool lp3944_dim_set_period(lp3944_t *p_lp3944, uint8_t dim, uint16_t period);

/**
 * [lp3944_dim_set_dutycycle description]
 *
 * @param     p_lp3944      [description]
 * @param     dim           [description]
 * @param     duty_cycle    [description]
 *
 * @return                  [description]
 */
bool lp3944_dim_set_dutycycle(lp3944_t *p_lp3944, uint8_t dim, uint8_t duty_cycle);

/**
 * [lp3944_dim_set_dutycycle_hex description]
 *
 * @param     p_lp3944    [description]
 * @param     dim         [description]
 * @param     value       [description]
 *
 * @return                [description]
 */
bool lp3944_dim_set_dutycycle_hex(lp3944_t *p_lp3944, uint8_t dim, uint8_t value);

#endif
