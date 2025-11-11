
#ifndef _smw_h_
#define _smw_h_

#include <stdint.h>

#ifndef smw_max_tasks
    #define smw_max_tasks 16
#endif

typedef struct
{
    void* context;
     /*replace callback with work function from other module?*/
    void (*callback)(void* _Context, uint64_t _MonTime)

} smw_task;


typedef struct
{
    smw_task tasks[smw_max_tasks];

} smw;

extern smw g_smw;

int smw_init();

smw_task* smw_create_task(void* _Context, void (*_Callback)(void* _Context, uint64_t _MonTime));

void smw_destroy_task(smw_task* _Task);

void smw_work(uint64_t _MonTime);

int smw_get_task_count();

void smw_dispose();

#endif


