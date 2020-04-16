/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __VEML6030_H__
#define __VEML6030_H__

#define VEML6030_ADDRESS_HIGH		(0x48)  // ADDR pin is High
#define VEML6030_ADDRESS_LOW		(0x10)  // ADDR pin is Low

#define VEML6030_REG_ALS_CONF		(0x00)
#define VEML6030_REG_ALS_WH			(0x01)
#define VEML6030_REG_ALS_WL			(0x02)
#define VEML6030_REG_POWER_SAVING	(0x03)
#define VEML6030_REG_ALS 			(0x04)
#define VEML6030_REG_WHITE 			(0x05)
#define VEML6030_REG_ALS_INT  		(0x06)

#define VEML6030_ALS_SD_POS			(0UL)
#define VEML6030_ALS_SD_MASK		(0x1UL << VEML6030_ALS_SD_POS)
#define VEML6030_ALS_SD_POWER_ON 	(0UL)
#define VEML6030_ALS_SD_POWER_OFF 	(1UL)

#define VEML6030_ALS_INT_EN_POS		(1UL)
#define VEML6030_ALS_INT_EN_MASK	(0x1UL << VEML6030_ALS_INT_EN_POS)
#define VEML6030_ALS_INT_EN_DISABLE (0UL)
#define VEML6030_ALS_INT_EN_ENABLE	(1UL)

#define VEML6030_ALS_PERS_POS		(4UL)
#define VEML6030_ALS_PERS_MASK		(0x03UL << VEML6030_ALS_PERS_POS)
#define VEML6030_ALS_PERS_1 		(0UL)
#define VEML6030_ALS_PERS_2			(1UL)
#define VEML6030_ALS_PERS_4			(2UL)
#define VEML6030_ALS_PERS_8			(3UL)

#define VEML6030_ALS_IT_POS			(6UL)
#define VEML6030_ALS_IT_MASK		(0x0FUL << VEML6030_ALS_IT_POS)
#define VEML6030_ALS_IT_25MS 		(0x1100UL)
#define VEML6030_ALS_IT_50MS		(0x1000UL)
#define VEML6030_ALS_IT_100MS		(0x0000UL)
#define VEML6030_ALS_IT_200MS		(0x0001UL)
#define VEML6030_ALS_IT_400MS		(0x0010UL)
#define VEML6030_ALS_IT_800MS		(0x0011UL)

#define VEML6030_ALS_GAIN_POS		(11UL)
#define VEML6030_ALS_GAIN_MASK		(0x03UL << VEML6030_ALS_GAIN_POS)
#define VEML6030_ALS_GAIN_1 		(0UL)
#define VEML6030_ALS_GAIN_2			(1UL)
#define VEML6030_ALS_GAIN_1_8 		(2UL)
#define VEML6030_ALS_GAIN_1_4		(3UL)

#define VEML6030_PSM_EN_POS			(0UL)
#define VEML6030_PSM_EN_MASK		(0x1UL << VEML6030_PSM_EN_POS)
#define VEML6030_PSM_EN_DISABLE 	(0UL)
#define VEML6030_PSM_EN_ENABLE 		(1UL)

#define VEML6030_PSM_POS			(1UL)
#define VEML6030_PSM_MASK			(0x03UL << VEML6030_PSM_POS)
#define VEML6030_PSM_MODE1 			(0UL)
#define VEML6030_PSM_MODE2 			(1UL)
#define VEML6030_PSM_MODE3 			(2UL)
#define VEML6030_PSM_MODE4 			(3UL)

#define VEML6030_INT_TH_HIGH_POS	(14UL)
#define VEML6030_INT_TH_HIGH_MASK	(0x01UL << VEML6030_INT_TH_HIGH_POS)

#define VEML6030_INT_TH_LOW_POS		(15UL)
#define VEML6030_INT_TH_LOW_MASK	(0x01UL << VEML6030_INT_TH_LOW_POS)

typedef struct veml6030_s veml6030_t;

struct veml6030_s
{
    uint8_t address;
     bool(*reg_write) (veml6030_t * p_veml6030, uint8_t addr, uint16_t value);
     bool(*reg_read) (veml6030_t * p_veml6030, uint8_t addr, uint16_t * p_value);
};

/**
 * [veml6030_conf_set description]
 *
 * @param     p_veml6030    [description]
 * @param     value         [description]
 *
 * @return                  [description]
 */
bool veml6030_conf_set(veml6030_t * p_veml6030, uint16_t value);

/**
 * [veml6030_psm_set description]
 *
 * @param     p_veml6030    [description]
 * @param     value         [description]
 *
 * @return                  [description]
 */
bool veml6030_psm_set(veml6030_t * p_veml6030, uint16_t value);

/**
 * [veml6030_power_on description]
 *
 * @param     p_veml6030    [description]
 *
 * @return                  [description]
 */
bool veml6030_power_on(veml6030_t * p_veml6030);

/**
 * [veml6030_power_off description]
 *
 * @param     p_veml6030    [description]
 *
 * @return                  [description]
 */
bool veml6030_power_off(veml6030_t * p_veml6030);

/**
 * [veml6030_als_data_read description]
 *
 * @param     p_veml6030    [description]
 * @param     p_value       [description]
 *
 * @return                  [description]
 */
bool veml6030_als_data_read(veml6030_t * p_veml6030, uint16_t * p_value);

#endif
