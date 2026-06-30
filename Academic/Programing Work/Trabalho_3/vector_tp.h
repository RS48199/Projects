#ifndef VECTOR_GET_IN_RANGE_H
#define VECTOR_GET_IN_RANGE_H

#include "thp.h"
#include "count_down.h"

int vector_get_in_range_with_thread_pool(int v[], int v_sz, int sv[], int min, int max, threadpool_t *tp);

#endif
