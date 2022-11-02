#include <stdio.h>
#include "bucketing_exhaust.h"
#include "bucketing.h"

int main()
{
    double default_value = 1000;
    int num_sampling_points = 10;
    double increase_rate = 2;

    printf("Creating bucketing state\n");
    bucketing_state* s = bucketing_state_create(default_value, num_sampling_points, increase_rate, 10);

    int prime = 7;
    int num = 2;
    int multiple = 2;
    int iters = 50;

    printf("Adding values\n");
    for (int i = 0; i < iters; ++i)
    {
        num = num * multiple % prime;
        //printf("iteration %d data value %d\n", i, num);
        bucketing_add(num, i + 1, s);
        //printf("value added\n");
        print_sorted_points(s->sorted_points);
        if (i >= num_sampling_points - 1)
        {
            //printf("Finding buckets\n");
            bucketing_exhaust_update_buckets(s);
            print_sorted_buckets(s->sorted_buckets);
        }
        printf("Predicting value %lf\n", bucketing_predict(-1, s));
        //printf("Sorted list length %d\n", list_length(s->sorted_points));
        printf("----------------------------------\n");
    }
    bucketing_state_delete(s);
    return 0;
}
