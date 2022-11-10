#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bucketing_manager.h"
#include "debug.h"

extern struct hash_table* info_of_resource_table;

int main(int argc, char** argv)
{
    category_mode_t mode;
    if (argc == 2)
    {
        if (strncmp(*(argv+1), "-greedy", 7) == 0)
            mode = CATEGORY_ALLOCATION_MODE_GREEDY_BUCKETING;
        else if (strncmp(*(argv+1), "-exhaust", 8) == 0)
            mode = CATEGORY_ALLOCATION_MODE_EXHAUSTIVE_BUCKETING;
        else
        {
            fatal("invalid bucketing mode\n");
            return 1;
        }
    }
    else
    {
        fatal("must specify bucketing mode\n");
        return 1;
    }
    double default_value;
    int num_sampling_points = 10;
    double increase_rate = 2;
    int max_num_buckets = 10;

    bucketing_manager_t* m = bucketing_manager_create(mode);
    char* res_names[3] = {"cores", "memory", "disk"};

    for (int i = 0; i < 3; ++i)
    {
        default_value = i == 0 ? 1 : 1000;
        bucketing_manager_add_resource_type(m, res_names[i], default_value, num_sampling_points, increase_rate, max_num_buckets);
    }

    int prime_mem = 7000;
    int num_mem = 2000;
    int prime_disk = 7000;
    int num_disk = 2000;
    int prime_core = 7;
    int num_core = 2;
    int multiple = 2;
    int iters = 50;

    struct rmsummary* task_r;
    struct rmsummary* pred_task_r;

    int task_id = 1;

    //printf("Adding values\n");
    for (int i = 0; i < iters; ++i)
    {
        task_r = rmsummary_create(0);
        num_core = num_core * multiple % prime_core;
        num_mem = num_mem * multiple % prime_mem;
        num_disk = num_disk * multiple % prime_disk;
        printf("iteration %d task w cores %d mem %d disk %d\n", i, num_core, num_mem, num_disk);

        while((pred_task_r = bucketing_manager_predict(m, task_id)))
        {
            if (pred_task_r->cores >= num_core && pred_task_r->memory >= num_mem && pred_task_r->disk >= num_disk)
            {
                break;
            }
        }

        rmsummary_set(task_r, res_names[0], num_core);
        rmsummary_set(task_r, res_names[1], num_mem);
        rmsummary_set(task_r, res_names[2], num_disk);
        bucketing_manager_add_task(m, task_id, task_r, 1);
        ++task_id;
        //printf("----------------------------------\n");
    }
    bucketing_manager_delete(m);
    hash_table_delete(info_of_resource_table);
    return 0;
}
