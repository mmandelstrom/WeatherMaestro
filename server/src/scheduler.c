#include "../include/scheduler.h"

Scheduler Global_Scheduler;

int scheduler_init()
{
	memset(&Global_Scheduler, 0, sizeof(Global_Scheduler));

	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		Global_Scheduler.tasks[i].context = NULL;
		Global_Scheduler.tasks[i].callback = NULL;
	}

	return 0;
}

Scheduler_Task* scheduler_create_task(void* _context, void (*_callback)(void* _context, uint64_t _montime))
{
	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		if(Global_Scheduler.tasks[i].context == NULL && Global_Scheduler.tasks[i].callback == NULL)
		{
			Global_Scheduler.tasks[i].context = _context;
			Global_Scheduler.tasks[i].callback = _callback;
			return &Global_Scheduler.tasks[i];
		}
	}

	return NULL;
}

void scheduler_destroy_task(Scheduler_Task* _Task)
{
	if(_Task == NULL)
		return;

	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		if(&Global_Scheduler.tasks[i] == _Task)
		{
			Global_Scheduler.tasks[i].context = NULL;
			Global_Scheduler.tasks[i].callback = NULL;
			break;
		}
	}
}

void scheduler_work(uint64_t _montime)
{

  uint64_t start = _montime;

	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		if(Global_Scheduler.tasks[i].callback != NULL)
			Global_Scheduler.tasks[i].callback(Global_Scheduler.tasks[i].context, _montime);

	}

  const uint64_t MIN_LOOP_MS = 1;
  uint64_t end = SystemMonotonicMS();
  uint64_t elapsed = end - start;

  if (elapsed < MIN_LOOP_MS) {
    ms_sleep(MIN_LOOP_MS - elapsed);
  }

}

int scheduler_get_task_count()
{
	int counter = 0;
	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		if(Global_Scheduler.tasks[i].callback != NULL)
			counter++;

	}

	return counter;
}

void scheduler_dispose()
{
	int i;
	for(i = 0; i < SCHEDULER_MAX_TASKS; i++)
	{
		Global_Scheduler.tasks[i].context = NULL;
		Global_Scheduler.tasks[i].callback = NULL;
	}
}
