/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "david61song",
    /* First member's full name */
    "david song",
    /* First member's email address */
    "none@none.none",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* defines size */
#define WSIZE 8 // 64-bit version
#define CHUNKSIZE (1<<12) // 2^12 = 4096

/* defines macro */

#define MAX(x,y) (x > y ? x : y)

#define GET(p) (*(uint64_t *)(p)) // deref
#define GET_SIZE(p) (GET(p) & ~0x7) // *(p) && (0x11111.....000) -> Only Get size
#define PACK(size, allocbit) (size || allocbit) // OR'ing size and allocation bit
#define CHECK(p) (GET(p) & 0x1) // *(p) && (0x0000....1) -> Only Get available bit

#define INSERT(idx, val) (*((uint64_t *) idx) = val, idx += WSIZE)
#define PUT(p, val) (*(uint64_t *)(p) = val) // deref and put
#define SET(p) (PUT(p, PACK(GET_SIZE(p), 1))) // set allocation bit
#define CLR(p) (PUT(p, PACK(GET_SIZE(p), 0))) // clear allocation bit

#define NEXT(p) ((char *)p + GET_SIZE(p)) // next block
#define PREV(p) ((char *)p - GET_SIZE(p - WSIZE)) // prev block
#define HDRP(bp) ((char *)bp - WSIZE) // returns header address
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - 2 * WSIZE) // returns footer address




/* global variables */
static char * heap_listp = 0;





/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/*
    Examples :
    ALIGN(7) -> 8
    ALIGN(6) -> 8
*/

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/*
    size_t -> system's maximum data size type
    SIZE_T_SIZE aligned size of size_t
*/


/*

[] -> 8 byte

[prologue][header][][][][][footer]...[header][][][][][footer][eplilogue]
*/


void *extend_heap(size_t words);
void *coalesce(void *bp);




int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    INSERT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(WSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(WSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == (void *) 0)
        return -1;
    return 0;
}

void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

void *coalesce(void *bp)
{
    size_t prev_alloc = CHECK(FTRP(PREV(bp)));
    size_t next_alloc = CHECK(HDRP(NEXT(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV(bp)), PACK(size, 0));
        bp = PREV(bp);
    }

    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV(bp))) + GET_SIZE(FTRP(NEXT(bp)));
        PUT(HDRP(PREV(bp)), PACK(size, 0));
        PUT(FTRP(NEXT(bp)), PACK(size, 0));
        bp = PREV(bp);
    }

    return bp;
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1)
	    return NULL;

    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














