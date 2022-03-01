/*
 * alloc.c
 *
 *  Created on: Mar 1, 2022
 *      Author: User
 */
#include "alloc.h"

typedef struct zbar_alloc_t
{
    struct
    {
        struct zbar_alloc_t *ptr;
        unsigned int size;
    } s;
    unsigned int align;
    unsigned int pad;
} zbar_alloc_t;

char gZbarAllocArray[ZBAR_ALLOC_ARRAY_SIZE];

static zbar_alloc_t gBase;
static zbar_alloc_t *gFreePtr = NULL;

unsigned int gAllocCount = 0;

void *zbar_malloc_t(size_t __size)
{
    char *__HEAP_START = gZbarAllocArray;
    char *__HEAP_END = __HEAP_START + ZBAR_ALLOC_ARRAY_SIZE;

    zbar_alloc_t *p, *prevp;
    unsigned nunits;

    zbar_alloc_t *bp = NULL;
    zbar_alloc_t *bprevp;

    nunits = ((__size + sizeof(zbar_alloc_t) - 1) / sizeof(zbar_alloc_t)) + 1;

    gAllocCount += nunits;

    if ((prevp = gFreePtr) == NULL)
    {
        p = (zbar_alloc_t *)
            __HEAP_START;
#if defined(_WIN32)
        p->s.size = (((intptr_t)__HEAP_END - (intptr_t)__HEAP_START) / sizeof(zbar_alloc_t));
#else
        p->s.size = (((unsigned int)__HEAP_END - (unsigned int)__HEAP_START) / sizeof(zbar_alloc_t));
#endif
        p->s.ptr = &gBase;
        gBase.s.ptr = p;
        gBase.s.size = 0;
        prevp = gFreePtr = &gBase;
    }

    for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr)
    {
        if (p->s.size == nunits)
        {
            prevp->s.ptr = p->s.ptr;
            gFreePtr = prevp;
            return (void *)(p + 1);
        }
        else if (p->s.size > nunits)
        {
            if (bp == NULL)
            {
                bp = p;
                bprevp = prevp;
            }

            if (bp->s.size > p->s.size)
            {
                bprevp = prevp;
                bp = p;
            }
        }

        if (p == gFreePtr)
        {
            if (bp != NULL)
            {
                gFreePtr = bprevp;
                p = bp;

                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;

                return (void *)(p + 1);
            }
            return NULL;
        }
    }
}

void zbar_free_t(void *__p)
{
    if (__p == NULL)
        return;

    zbar_alloc_t *bp, *p;

#if defined(_WIN32)
    if (__p == NULL || (__p < (void *)&gZbarAllocArray[0]) || (__p > (void *)(&gZbarAllocArray[ZBAR_ALLOC_ARRAY_SIZE] - sizeof(zbar_alloc_t))))
#else
    if (__p == NULL || (__p < &gZbarAllocArray[0]) || (__p > (&gZbarAllocArray[ZBAR_ALLOC_ARRAY_SIZE] - sizeof(zbar_alloc_t))))
#endif
        return;

    bp = (zbar_alloc_t *)__p - 1; /* point to block header */

    gAllocCount -= bp->s.size;
    for (p = gFreePtr; !((bp > p) && (bp < p->s.ptr)); p = p->s.ptr)
    {
        if ((p >= p->s.ptr) && ((bp > p) || (bp < p->s.ptr)))
        {
            break; /* freed block at start or end of arena */
        }
    }

    if ((bp + bp->s.size) == p->s.ptr)
    {
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    }
    else
    {
        bp->s.ptr = p->s.ptr;
    }

    if ((p + p->s.size) == bp)
    {
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    }
    else
    {
        p->s.ptr = bp;
    }

    gFreePtr = p;
}

void *zbar_calloc_t(size_t __nmemb, size_t __size)
{
    void *p = zbar_malloc(__nmemb * __size);
    if (p != NULL)
    {
        memset((char *)p, 0, __nmemb * __size);
    }
    return p;
}

void *zbar_realloc_t(void *__p, size_t __size)
{
    zbar_alloc_t *bp, *p, *np;

    unsigned nunits;
    unsigned aunits;
    unsigned int oldsize;

    if (__p == NULL)
    {
        bp = zbar_malloc(__size);
        return bp;
    }

    if (__size == 0)
    {
        zbar_free(__p);
        return NULL;
    }
    nunits = ((__size + sizeof(zbar_alloc_t) - 1) / sizeof(zbar_alloc_t)) + 1;

    bp = (zbar_alloc_t *)__p - 1; /* point to block header */
    if (nunits <= bp->s.size)
    {
        return __p;
    }

    oldsize = (bp->s.size - 1) * sizeof(zbar_alloc_t);
    bp = zbar_malloc(__size);
    memcpy(bp, __p, oldsize);
    zbar_free(__p);
    return bp;
}
