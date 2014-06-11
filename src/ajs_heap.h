#ifndef _AJS_HEAP_H
#define _AJS_HEAP_H
/**
 * @file  A pool based memory allocator designed for embedded systemms.
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <ajtcl/aj_target.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Indicates that allocations can be borrowed from the the next larger pool if the best-fit pool is
 * exhausted.
 */
#define AJS_POOL_BORROW   1

typedef struct _AJS_HeapConfig {
    const uint16_t size;     /* Size of the pool entries in bytes */
    const uint16_t entries;  /* Number of entries in this pool */
    const uint8_t borrow;    /* Indicates if pool can borrow from then next larger pool */
    const uint8_t heapIndex; /* What heap memory location to use for this pool */
} AJS_HeapConfig;

/*
 * This should be 4 or 8
 */
#define AJS_HEAP_POOL_ROUNDING  4

/**
 * Example of a heap pool description. Note that the pool sizes must be in ascending order of size
 * and should be rounded according to AJ_HEAP_POOL_ROUNDING.

   @code

   static const AJS_HeapConfig memPools[] = {
    { 32,   1, AJS_POOL_BORROW },
    { 96,   4, },
    { 192,  1, }
   };

   @endcode

 */


/**
 * Compute the required size of the heap for the given pool list.
 *
 * @param heapConfig Description of the pools to require.
 * @param numPools   The number of different sized memory pools, maximum is 255.
 *
 * @return  Returns the total heap size required.
 */
size_t AJS_HeapRequired(const AJS_HeapConfig* heapConfig, uint8_t numPools, uint8_t heapNum);

/**
 * Initialize the heap.
 *
 * @param heap       Pointer to the heap storage
 * @param heapSz     Size of the heap for sanity checking
 * @param heapConfig Description of the pools to allocate. This structure must remain valid for the
 *                   lifetime of the heap
 * @param numPools   The number of different sized memory pools, maximum is 255.
 *
 * @return - AJ_OK if the heap was allocated and pools were initialized
 *         - AJ_ERR_RESOURCES of the heap is not big enough to allocate the requested pools.
 */
AJ_Status AJS_HeapInit(void** heap, size_t* heapSz, const AJS_HeapConfig* heapConfig, uint8_t numPools, uint8_t numHeaps);

/**
 * Terminate the heap
 */
void AJS_HeapTerminate(void* heap);

/**
 * Indicates if the heap has been initialized
 */
uint8_t AJS_HeapIsInitialized();

#ifndef NDEBUG
void AJS_HeapDump(void);
#else
#define AJS_HeapDump()
#endif
#ifdef __cplusplus
}
#endif
#endif
