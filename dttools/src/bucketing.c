#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "bucketing.h"
#include "list.h"
#include "random.h"

//TODO: integrate debug in all bucketing files

bucketing_point* bucketing_point_create(double val, double sig)
{
    bucketing_point* p = malloc(sizeof(*p));
    
    if (p == NULL) 
        return p;
    

    p->val = val;
    p->sig = sig;
    return p;
}

void bucketing_point_delete(bucketing_point *p)
{
    free(p);
}

bucketing_bucket* bucketing_bucket_create(double val, double prob)
{
    bucketing_bucket* b = malloc(sizeof(*b));

    if (b == NULL)
        return b;

    b->val = val;
    b->prob = prob;
    return b;
}

void bucketing_bucket_delete(bucketing_bucket* b)
{
    free(b);
}

bucketing_state* bucketing_state_create(double default_value, int num_sampling_points,
    double increase_rate, int max_num_buckets)
{
    bucketing_state* s = malloc(sizeof(*s));

    if (s == NULL)
        return s;

    s->sorted_points = list_create();
    s->sequence_points = list_create();
    s->sorted_buckets = list_create();
    s->num_points = 0;
    s->in_sampling_phase = 1;
    s->prev_op = null;
    s->default_value = default_value;
    s->num_sampling_points = num_sampling_points;
    s->increase_rate = increase_rate;
    s->max_num_buckets = max_num_buckets;

    return s;
}

void bucketing_state_delete(bucketing_state* s)
{
    list_free(s->sorted_points);
    list_delete(s->sorted_points);
    /* pointers already free'd as sorted_points and sequence_points
     * share the same set of pointers */
    list_delete(s->sequence_points);
    list_free(s->sorted_buckets);
    list_delete(s->sorted_buckets);
    free(s);
}

bucketing_cursor_w_pos* bucketing_cursor_w_pos_create(struct list_cursor* lc, int pos)
{
    bucketing_cursor_w_pos* cursor_pos = malloc(sizeof(*cursor_pos));
    
    if (!cursor_pos)
        return cursor_pos;
    
    cursor_pos->lc = lc;
    cursor_pos->pos = pos;

    return cursor_pos;
}

void bucketing_cursor_w_pos_delete(bucketing_cursor_w_pos* cursor_pos)
{
    list_cursor_destroy(cursor_pos->lc);
    free(cursor_pos);
}

bucketing_bucket_range* bucketing_bucket_range_create(int lo, int hi, struct list* l)
{
    bucketing_bucket_range* range = malloc(sizeof(*range));
    
    if (!range)
        return range;
    
    struct list_cursor* cursor_lo = list_cursor_create(l);
    if (!list_seek(cursor_lo, lo))
        return NULL;
    bucketing_cursor_w_pos* cursor_pos_lo = bucketing_cursor_w_pos_create(cursor_lo, lo);
    range->lo = cursor_pos_lo;
    
    struct list_cursor* cursor_hi = list_cursor_create(l);
    if (!list_seek(cursor_hi, hi))
        return NULL;
    bucketing_cursor_w_pos* cursor_pos_hi = bucketing_cursor_w_pos_create(cursor_hi, hi);
    range->hi = cursor_pos_hi;
    return range;
}

void bucketing_bucket_range_delete(bucketing_bucket_range* range)
{
    bucketing_cursor_w_pos_delete(range->lo);
    bucketing_cursor_w_pos_delete(range->hi);
    free(range);
}

int bucketing_add(double val, double sig, bucketing_state* s)
{
    /* insert to sorted list and append to sequence list */
    bucketing_point *p = bucketing_point_create(val, sig);
    bucketing_insert_point_to_sorted_list(s->sorted_points, p);
    list_push_tail(s->sequence_points, p);
    
    /* Change to predicting phase if appropriate */
    s->num_points++;
    if (s->num_points >= s->num_sampling_points)
    {
        s->in_sampling_phase = 0;
    }

    /* set previous operation */
    s->prev_op = add;
    
    return 0;
}

double bucketing_predict(double prev_val, bucketing_state* s)
{
    /* in sampling phase */
    s->prev_op = predict;
    if (s->in_sampling_phase)
    {
        /* if new, return default value */
        if (prev_val == -1)
            return s->default_value;
        /* otherwise increase to exponent level */
        else
        {
            int exp = floor(log(prev_val/s->default_value)/log(s->increase_rate)) + 1;
            return s->default_value * pow(s->increase_rate, exp);
        } 
    }

    struct list_cursor* lc = list_cursor_create(s->sorted_buckets); //cursor to iterate
    list_seek(lc, 0);               //reset to 0
    bucketing_bucket* bb_ptr = 0;   //pointer to hold item from list
    double sum = 0;                 //sum of probability
    double ret_val;                 //predicted value to be returned
    int exp;                        //exponent to raise if prev_val > max_val
    double rand = random_double();  //random double to choose a bucket

    /* Loop through list of buckets to choose 1 */
    for (unsigned int i = 0; i < list_length(s->sorted_buckets); ++i, list_next(lc))
    {
        list_get(lc, (void**) &bb_ptr);

        /* return if at last bucket */
        if (i == list_length(s->sorted_buckets) - 1)
        {
            ret_val = bb_ptr->val;
            if (ret_val <= prev_val)
            {
                exp = floor(log(prev_val/s->default_value)/log(s->increase_rate)) + 1;
                list_cursor_destroy(lc);
                return s->default_value * pow(s->increase_rate, exp);   
            }
            list_cursor_destroy(lc);
            return ret_val;
        }

        sum += bb_ptr->prob;

        if (sum > rand)
        {
            ret_val = bb_ptr->val;
            list_cursor_destroy(lc);
            return ret_val;
        }
    }

    return -1;  //control should never reach here
}

int bucketing_insert_point_to_sorted_list(struct list* l, bucketing_point *p)
{
    struct list_cursor* lc = list_cursor_create(l);
    
    /* If list is empty, append new point to list */
    if (list_length(l) == 0)
    {
        list_insert(lc, p);
        list_cursor_destroy(lc);
        return 0;
    }

    /* Linear insert a data point */
    list_seek(lc, 0);
    bucketing_point* bpp = 0;
    int inserted = 0;
    do
    {
        list_get(lc, (void**) &bpp);
        if (bpp->val >= p->val)
        {
            list_insert(lc, p);
            inserted = 1;
            break;
        }
    }
    while (list_next(lc));

    /* Append point if it isn't inserted */
    if (inserted == 0)
    {
        list_insert(lc, p);
    }

    list_cursor_destroy(lc);
    return 0;
}

int bucketing_cursor_pos_list_clear(struct list* l, void (*f) (bucketing_cursor_w_pos*))
{
    bucketing_cursor_w_pos* tmp;
    
    while ((tmp = list_pop_head(l)))
        f(tmp);
    
    return 0;
}

int bucketing_bucket_range_list_clear(struct list* l, void (*f) (bucketing_bucket_range*))
{
    bucketing_bucket_range* tmp;
    
    while ((tmp = list_pop_head(l)))
        f(tmp);
    
    return 0;
}

struct list* bucketing_cursor_pos_list_sort(struct list* l, int (*f) (const void*, const void*))
{
    unsigned int size = list_length(l);
    unsigned int i = 0;
    bucketing_cursor_w_pos** arr = malloc(size * sizeof(*arr));
    struct list_cursor* lc = list_cursor_create(l);
    list_seek(lc, 0);

    /* Save all elements to array */
    while (list_get(lc, (void**) &arr[i]))
    {
        ++i;
        list_next(lc);
    }

    /* Destroy the list but not its elements */
    list_cursor_destroy(lc); 
    list_delete(l);
    
    /* Qsort the array */
    qsort(arr, size, sizeof(*arr), f);

    struct list* ret = list_create();
    lc = list_cursor_create(ret);

    /* Put back elements to a new list */
    for (i = 0; i < size; ++i)
        list_insert(lc, arr[i]);
    
    list_cursor_destroy(lc);
    free(arr);

    return ret;
}

int compare_break_points(const void* p1, const void* p2)
{
    return (*((bucketing_cursor_w_pos**) p1))->pos - (*((bucketing_cursor_w_pos**) p2))->pos;
}
