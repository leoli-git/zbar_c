/*
 * alloc.h
 *
 *  Created on: Mar 1, 2022
 *      Author: User
 */

#ifndef ZBAR_ALLOC_H_
#define ZBAR_ALLOC_H_

#include <stdlib.h>
#include <string.h>

#define ZBAR_ALLOC_ARRAY_SIZE (114 * 1024)

void *zbar_malloc_t(size_t __size);
void zbar_free_t(void *__p);
void *zbar_calloc_t(size_t __nmemb, size_t __size);
void *zbar_realloc_t(void *__p, size_t __size);

#if 0
#define zbar_malloc malloc
#define zbar_free free
#define zbar_calloc calloc
#define zbar_realloc realloc
#else
#define zbar_malloc zbar_malloc_t
#define zbar_free zbar_free_t
#define zbar_calloc zbar_calloc_t
#define zbar_realloc zbar_realloc_t
#endif

#endif /* ZBAR_ALLOC_H_ */
