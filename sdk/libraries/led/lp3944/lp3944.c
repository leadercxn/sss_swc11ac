/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "lp3944.h"

/**
 * Set the led status
 *
 * @led: a lp3944_led_data structure
 * @status: one of LP3944_LED_STATUS_OFF
 *                 LP3944_LED_STATUS_ON
 *                 LP3944_LED_STATUS_DIM0
 *                 LP3944_LED_STATUS_DIM1
 */
bool lp3944_led_set(lp3944_t *p_lp3944, uint8_t led, uint8_t status)
{
  uint8_t id = led;
	uint8_t reg;
	uint8_t val = 0;

	switch (id)
  {
    case LP3944_LED0:
    case LP3944_LED1:
    case LP3944_LED2:
    case LP3944_LED3:
      reg = LP3944_REG_LS0;
      break;
    case LP3944_LED4:
    case LP3944_LED5:
    case LP3944_LED6:
    case LP3944_LED7:
      id -= LP3944_LED4;
      reg = LP3944_REG_LS1;
      break;
    default:
      return false;
	}

	if (status > LP3944_LED_STATUS_DIM1)
  {
		return false;
  }

	if(!p_lp3944->reg_read(p_lp3944, reg, &val))
  {
    return false;
  }

	val &= ~(LP3944_LED_STATUS_MASK << (id << 1));
	val |= (status << (id << 1));

  if(!p_lp3944->reg_write(p_lp3944, reg, val))
  {
    return false;
  }

	return true;
}

/**
 * Set the period for DIM status
 *
 * @client: the i2c client
 * @dim: either LP3944_DIM0 or LP3944_DIM1
 * @period: period of a blink, that is a on/off cycle, expressed in ms.
 */
bool lp3944_dim_set_period(lp3944_t *p_lp3944, uint8_t dim, uint16_t period)
{
	uint8_t psc_reg;
	uint8_t psc_value;

	if (dim == LP3944_DIM0)
		psc_reg = LP3944_REG_PSC0;
	else if (dim == LP3944_DIM1)
		psc_reg = LP3944_REG_PSC1;
	else
		return false;;

	/* Convert period to Prescaler value */
	if (period > LP3944_PERIOD_MAX)
		return false;;

	psc_value = (period * 255) / LP3944_PERIOD_MAX;

  if(!p_lp3944->reg_write(p_lp3944, psc_reg, psc_value))
  {
    return false;
  }

	return true;
}

/**
 * Set the duty cycle for DIM status
 *
 * @client: the i2c client
 * @dim: either LP3944_DIM0 or LP3944_DIM1
 * @duty_cycle: percentage of a period during which a led is ON
 */
bool lp3944_dim_set_dutycycle(lp3944_t *p_lp3944, uint8_t dim, uint8_t duty_cycle)
{
	uint8_t pwm_reg;
	uint8_t pwm_value;

	if (dim == LP3944_DIM0)
		pwm_reg = LP3944_REG_PWM0;
	else if (dim == LP3944_DIM1)
		pwm_reg = LP3944_REG_PWM1;
	else
		return false;

	/* Convert duty cycle to PWM value */
	if (duty_cycle > LP3944_DUTY_CYCLE_MAX)
		return false;

	pwm_value = (duty_cycle * 255) / LP3944_DUTY_CYCLE_MAX;

  if(!p_lp3944->reg_write(p_lp3944, pwm_reg, pwm_value))
  {
    return false;
  }

	return true;
}

/**
 * [lp3944_dim_set_dutycycle description]
 *
 * @param     p_lp3944      [description]
 * @param     dim           [description]
 * @param     duty_cycle    [description]
 *
 * @return                  [description]
 */
bool lp3944_dim_set_dutycycle_hex(lp3944_t *p_lp3944, uint8_t dim, uint8_t value)
{
  uint8_t pwm_reg;

  if (dim == LP3944_DIM0)
    pwm_reg = LP3944_REG_PWM0;
  else if (dim == LP3944_DIM1)
    pwm_reg = LP3944_REG_PWM1;
  else
    return false;

  if(!p_lp3944->reg_write(p_lp3944, pwm_reg, value))
  {
    return false;
  }

  return true;
}

