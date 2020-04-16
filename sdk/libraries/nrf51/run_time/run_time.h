/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */
 
#ifndef __RUN_TIME_H_
#define __RUN_TIME_H_

/**
 * @brief Function for initializing run-time
 */
void RunTimeInit(void);

/**
 * @brief Function for getting time from system on in ms
 *
 * @retval  Timestamp in ms
 */
uint32_t RunTimeGet(void);

/**
 * @brief Function for getting elapsed time from one past timestamp to now 
 *
 * @param[i] past  The past timestamp
 *
 * @retval  The elaped time in ms
 */
uint32_t RunTimeElapsedGet(uint32_t past);

#endif
