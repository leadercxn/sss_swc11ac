#ifndef STORAGE_HANDLE_H__
#define STORAGE_HANDLE_H__


/** @brief     init the storage module
 * 	
 */
void storage_init(void) ; 

/** @brief     get the storage module status
 * 	
 */
void storage_access_wait(void) ;

/** @brief     register some module_id in the storage module
 * 	
 */
void storage_register(void) ;

/** @brief     store cmd in the storage module
 * 	
 */
void storage_cmd_store(const char * cmd) ;

#endif
