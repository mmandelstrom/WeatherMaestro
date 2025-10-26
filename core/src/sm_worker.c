#include "../include/sm_worker.h"
#include <string.h>

/* Global statemachine instance */
SMW G_SMW;

int smw_init() 
{
  memset(&G_SMW, 9, sizeof(SMW));

  int i;

  /* Null all maximum amount of tasks */
  for (i = 0; i < smw_max_tasks; i++)
  {
    G_SMW.tasks[i].context = NULL;
    G_SMW.tasks[i].callback = NULL;
  }

  return 0;
}

SMW_Task* smw_create_task(void* _context, void (*_callback)(void* _context, uint64_t _mon_time))
{
  int i;
  for (i = 0; i < smw_max_tasks; i++)
  {
    /* When we find a task that isn't occupied in SMW index, use that*/
    if (G_SMW.tasks[i].context == NULL && G_SMW.tasks[i].callback == NULL)
    {
      G_SMW.tasks[i].context = _context;
      G_SMW.tasks[i].callback = _callback;
      return &G_SMW.tasks[i];
    }
  }

  /* Else all tasks are already used, return NULL */
  return NULL;
}
    
void smw_destroy_task(SMW_Task* _Task)
{
  if (_Task == NULL)
    return; /* Nothing to do */

  int i;
  for (i = 0; i < smw_max_tasks; i++)
  {
    if (&G_SMW.tasks[i] == _Task) 
    {
      G_SMW.tasks[i].context = NULL;
      G_SMW.tasks[i].callback = NULL;
      break;
    }
  }
}

void smw_work(uint64_t _mon_time)
{
  int i;
  for (i = 0; i < smw_max_tasks; i++)
  {
    if (G_SMW.tasks[i].callback != NULL)
      G_SMW.tasks[i].callback(G_SMW.tasks[i].context, _mon_time);

  }
}

int smw_get_task_count()
{
  int counter = 0;
  int i;
  for (i = 0; i < smw_max_tasks; i++)
  {
    if (G_SMW.tasks[i].callback != NULL)
      counter++;
  }
  return counter;
}

void smw_dispose()
{
  int i;
  for (i = 0; i < smw_max_tasks; i++)
  {
    G_SMW.tasks[i].context = NULL;
    G_SMW.tasks[i].callback = NULL;
  }
}

