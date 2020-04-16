/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

/**
 * Function for initializing storage module.
 */
void storage_init(void);

/**
 * Function for waiting all storage processing done.
 */
void storage_access_wait(void);

#endif
