/* StateMachineWorker
 * StateMachineScheduler
 * TaskScheduler
 * */

#ifndef __SMW_H_
#define __SMW_H_

#include <stdint.h>

#ifndef smw_max_tasks
	#define smw_max_tasks 16
#endif

typedef struct
{
	void* context;
	void (*callback)(void* context, uint64_t mon_time);

} SMW_Task;


typedef struct
{
	SMW_Task tasks[smw_max_tasks];

} SMW;


extern SMW g_smw;


int smw_init();

/** 
 * Returns null if max tasks are already used */
SMW_Task* smw_create_task(void* _context, void (*_callback)(void* _context, uint64_t _mon_time));

void smw_destroy_task(SMW_Task* _Task);

void smw_work(uint64_t _mon_time);

int smw_get_task_count();

void smw_dispose();

#endif 
