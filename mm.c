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
#define DSIZE (WSIZE * 2)
#define CHUNKSIZE (1<<12) // 2^12 = 4096

/* defines macro */

#define MAX(x,y) (x > y ? x : y)

#define GET(p) (*(uint64_t *)(p)) // deref
#define GET_SIZE(p) (GET(p) & ~0x7) // *(p) && (0x11111.....000) -> Only Get size
#define PACK(size, alloc) ((size) | (alloc)) // OR'ing size and allocation bit
#define CHECK(p) (GET(p) & 0x1) // *(p) && (0x0000....1) -> Only Get available bit
#define PUT(p, val) (*(uint64_t *)(p) = val) // deref and put

#define SET(p) (PUT(p, PACK(GET_SIZE(p), 1))) // set allocation bit
#define CLR(p) (PUT(p, PACK(GET_SIZE(p), 0))) // clear allocation bit

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define HDRP(bp) ((char *)bp - WSIZE) // returns header address
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE) // returns footer address




/* global variables */
static char *heap_listp = 0;
static char *start = 0;

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




int mm_init(void) {
    /* Initialize an empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    start = heap_listp;

    /* Set up the heap with initial padding, prologue, and epilogue blocks */
    PUT(heap_listp, 0);                            // Alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(WSIZE, 1)); // Prologue header: size=8, allocated=1
    PUT(heap_listp + (2 * WSIZE), PACK(WSIZE, 1)); // Prologue footer: size=8, allocated=1
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // Epilogue header: size=0, allocated=1
    heap_listp += (2 * WSIZE);                     // Position heap pointer after prologue

    /* Extend the heap with a free block to start */
    if (extend_heap(CHUNKSIZE / WSIZE) == (void *) 0)
        return -1;
    return 0;

/*
   Heap Layout at Initialization:
   | Offset | Content   | Description         |
   |--------|-----------|---------------------|
   | 0      | [padding] | Alignment padding   |
   | 8      | [8/1]     | Prologue header     |
   | 16     | [8/1]     | Prologue footer     |
   | 24     | [0/1]     | Epilogue header     |
   | 32     | 0         | end of heap(brk)    |
   Note: [size/allocated flag], measured in bytes
*/

}



void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((uint64_t)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); /* Free block header (old epilogue eliminated) */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer (move backward by size), 0 means not allocated!*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);

/*
   Heap Layout after Extending by 512 bytes:
   | Offset | Content        | Description                |
   |--------|----------------|----------------------------|
   | 0      | [padding]      | Alignment padding          |
   | 8      | [8/1]          | Prologue header            |
   | 16     | [8/1]          | Prologue footer            |
   | 24     | [4096/0]       | Free block header          |
   | 32     | ...            | Free block payload  ...    |
   | 4112   | [4096/0]       | Free block footer          |
   | 4120   | [0/1]          | New epilogue header        |
   | 4128   |  0             | end of the heap (brk)      |
   Note: [size/allocated flag], measured in bytes.
*/


}

void *coalesce(void *bp)
{
    size_t prev_alloc = CHECK(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = CHECK(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

void *find_fit(size_t size){
    char *curr = heap_listp;

    while (GET_SIZE(curr) != 0){
        curr = NEXT_BLKP(curr);
    }



    return (void *) curr;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size) {
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}



/*
 * mm_free - Freeing a block
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
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














