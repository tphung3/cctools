#ifndef BUCKETING_H
#define BUCKETING_H

#include "list.h"

/* Bucketing has two operations, add and predict */
typedef enum
{
    BUCKETING_OP_ADD = 0,
    BUCKETING_OP_PREDICT,
    BUCKETING_OP_NULL    //only used when initializing
} bucketing_operation;

/* Each point (e.g., a task) in a bucket is a pair of value 
 * (e.g., memory consumption) and significance 
 * (encoding relative time position of task) */
typedef struct
{
    /* value */
    double val;

    /* significance */
    double sig;
} bucketing_point;

/* Each bucket is a pair of value (top delimiter) and probability 
 * that the next task falls into its range (lo, hi) where lo is
 * the top delimiter of the right below bucket (or zero if bucket 
 * is the lowest) and hi is value */
typedef struct
{
    /* value */
    double val;

    /* probability */
    double prob;
} bucketing_bucket;

/* State of the bucket */
typedef struct
{
    /** Begin: internally maintained fields **/
    /* a doubly linked list of pointers to points of type 'bucketing_point'
     * sorted by 'point->val' in increasing order
     * sorted_points and sequence_points share the same set of pointers */
    struct list *sorted_points;

    /* a doubly linked list of pointers to points of type 'bucketing_point'
     * sorted by 'point->sig' in increasing order
     * sequence_points and sorted_points share the same set of pointers */
    struct list *sequence_points;
    
    /* a doubly linked list of pointers to buckets of type 'bucketing_bucket'
     * sorted by 'bucket->val' in increasing order */
    struct list *sorted_buckets;
    
    /* total number of points */
    int num_points;
    
    /* whether bucketing is in sampling phase, 1 is yes, 0 is no */
    int in_sampling_phase; 
    
    /* track previous operation, this helps with the decision to find
     * buckets or not. This is -1 in the beginning as there's no previous
     * operation. */
    bucketing_operation prev_op;

    /** End: internally maintained fields **/

    /** Begin: externally provided fields **/
    /* default value to be used in sampling phase */
    double default_value;

    /* number of points needed to transition from sampling to predicting phase */
    int num_sampling_points;
    
    /* rate to increase a value when in sampling phase or exceeded max value in 
     * predicting phase */
    double increase_rate;

    /* the maximum number of buckets to break (only exhaustive bucketing */
    int max_num_buckets;

    /** End: externally provided fields **/ 
} bucketing_state;

/** Begin: APIs **/

/* Create a bucketing state
 * @param default_value default value in sampling state
 * @param num_sampling_points number of needed sampling points
 * @param increase_rate rate to increase values
 * @param max_num_buckets the maximum number of buckets to find (only for exhaustive bucketing)
 * @return pointer to created bucketing state
 * @return NULL if failure */
bucketing_state* bucketing_state_create(double default_value, int num_sampling_points,
    double increase_rate, int max_num_buckets);

/* Delete a bucketing state
 * @param s pointer to bucketing state to be deleted */
void bucketing_state_delete(bucketing_state* s);

/* Add a point
 * @param val value of point to be added
 * @param sig significance of point to be added
 * @param s the relevant bucketing state
 * @return 0 if success
 * @return 1 if failure */
int bucketing_add(double val, double sig, bucketing_state* s);

/* Predict a value, only predict when we need a new value, don't predict when prev value
 * (if available) is usable
 * @param prev_val previous value to consider, -1 if no previous value, 
 * > 0 means a larger value is expected from prediction
 * @param s the relevant bucketing_state
 * @return the predicted value
 * @return -1 if failure */
double bucketing_predict(double prev_val, bucketing_state* s);

/* Create a bucketing bucket
 * @param val value of bucket
 * @param prob probability of bucket
 * @return pointer to created bucket
 * @return NULL if failure */
bucketing_bucket* bucketing_bucket_create(double val, double prob);

/* Delete a bucketing bucket
 * @param b the bucket to be deleted */
void bucketing_bucket_delete(bucketing_bucket* b);

/** End: APIs **/

/** Begin: debug functions **/

/* Print a sorted list of bucketing_bucket
 * @param l the list of buckets */
void bucketing_sorted_buckets_print(struct list* l);

/* Print a sorted list of bucketing_point
 * @param l the list of points */
void bucketing_sorted_points_print(struct list* l);

/** End: debug functions **/

#endif
