#ifndef __SW_CFG_H
#define __SW_CFG_H


#define SCHED_MAX_EVENT_DATA_SIZE      	    sizeof(app_timer_event_t)           /**< Maximum size of scheduler events. Note that scheduler BLE stack events do not contain any data, as the events are being pulled from the stack in the event handler. */
#define SCHED_QUEUE_SIZE               	    20 

#define  APP_TIMER_OP_QUEUE_SIZE            20                                  //定时器操作队列的大小 

#define CENTRAL_LINK_COUNT                  0                                   /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT               1                                   /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/
#define IS_SRVC_CHANGED_CHARACT_PRESENT     1                                   /**< Include the service changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */






#endif


















