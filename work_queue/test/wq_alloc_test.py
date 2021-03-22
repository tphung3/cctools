#! /usr/bin/env python

# work queue python binding tests
# tests for missing/recursive inputs/outputs.

import sys
import work_queue as wq

def check_task(category, category_mode, max, min, expected):
    q.specify_category_max_resources(category, max)
    q.specify_category_min_resources(category, min)
    q.specify_category_mode(category, category_mode)

    t = wq.Task('/bin/echo hello')
    t.specify_category(category)
    q.submit(t)

    t = q.wait(5)
    if not t:
        print("Task did not complete in the allotted time.")
        sys.exit(1)
    else:
        print("{}:".format(category))
        print("expected: {}".format(expected))
        print("allocated: {}".format({
            'cores': t.resources_allocated.cores,
            'memory': t.resources_allocated.memory,
            'disk': t.resources_allocated.disk,
            'gpus': t.resources_allocated.gpus}))

        assert(t.resources_allocated.cores == expected['cores'])
        assert(t.resources_allocated.memory == expected['memory'])
        assert(t.resources_allocated.disk == expected['disk'])
        assert(t.resources_allocated.gpus == expected['gpus'])


port_file = None
try:
    port_file = sys.argv[1]
    (worker_cores, worker_memory, worker_disk, worker_gpus) = (int(sys.argv[i]) for i in range(2,6))
except IndexError:
    sys.stderr.write("Usage: {} PORTFILE WORKER_CORES WORKER_MEMORY WORKER_DISK\n".format(sys.argv[0]))


q = wq.WorkQueue(0)
with open(port_file, 'w') as f:
    print('Writing port {port} to file {file}'.format(port=q.port, file=port_file))
    f.write(str(q.port))

print(wq.__file__)

r = {'cores': 1, 'memory': 2, 'disk': 3, 'gpus': 4}
check_task('all_specified', wq.WORK_QUEUE_ALLOCATION_MODE_FIXED, max = r, min = {}, expected = r)

check_task('all_specified_no_gpu',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'cores': 1, 'memory': 2, 'disk': 3},
        min = {},
        expected = {'cores': 1, 'memory': 2, 'disk': 3, 'gpus': 0})

check_task('all_specified_no_cores',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'gpus': 4, 'memory': 2, 'disk': 3},
        min = {},
        expected = {'cores': 0, 'memory': 2, 'disk': 3, 'gpus': 4})

check_task('all_zero',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'cores': 0, 'memory': 0, 'disk': 0, 'gpus': 0},
        min = {},
        expected = {'cores': worker_cores, 'memory': worker_memory, 'disk': worker_disk, 'gpus': 0})

check_task('only_memory',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'memory': worker_memory/2},
        min = {},
        expected = {'cores': worker_cores/2, 'memory': worker_memory/2, 'disk': worker_disk/2, 'gpus': 0})

check_task('only_cores',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'cores': worker_cores},
        min = {},
        expected = {'cores': worker_cores, 'memory': worker_memory, 'disk': worker_disk, 'gpus': 0})

check_task('only_memory_w_minimum',
        wq.WORK_QUEUE_ALLOCATION_MODE_FIXED,
        max = {'memory': worker_memory/2},
        min = {'cores': 3, 'gpus': 2},
        expected = {'cores': 3, 'memory': worker_memory/2, 'disk': worker_disk/2, 'gpus': 2})

check_task('auto_whole_worker',
        wq.WORK_QUEUE_ALLOCATION_MODE_MIN_WASTE,
        max = {},
        min = {},
        expected = {'cores': worker_cores, 'memory': worker_memory, 'disk': worker_disk, 'gpus': 0})

q.specify_category_first_allocation_guess('auto_with_guess', {'cores': 1, 'memory': 2, 'disk': 3})
check_task('auto_with_guess',
        wq.WORK_QUEUE_ALLOCATION_MODE_MIN_WASTE,
        max = {},
        min = {},
        expected = {'cores': 1, 'memory': 2, 'disk': 3, 'gpus': 0})

