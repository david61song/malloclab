/*
 * mm-explicit.c - The fastest, least memory-efficient malloc package.
 *
 * In this approach
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

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

/*

* why we need boundary tag? --> for coalescing.











*/

/* defines size */
#define WSIZE 8 // 64-bit version
#define DSIZE (WSIZE * 2)
#define CHUNKSIZE (1<<12) // 2^12 = 4096

/* defines macro */

#define MAX(x,y) (x > y ? x : y)
#define ABS(x) (x > 0 ? x : x * -1)

#define GET(p) (*(uint64_t *)(p)) // deref
#define GET_SIZE(p) (GET(p) & ~0x7) // *(p) && (0x11111.....000) -> Only Get size
#define PACK(size, alloc) ((size) | (alloc)) // OR'ing size and allocation bit
#define CHECK(p) (GET(p) & 0x1) // *(p) && (0x0000....1) -> Only Get available bit
#define PUT(p, val) (*(uint64_t *)(p) = val) // deref and put

#define SET(p) (PUT(p, PACK(GET_SIZE(p), 1))) // set allocation bit
#define CLR(p) (PUT(p, PACK(GET_SIZE(p), 0))) // clear allocation bit

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //always pointing (real - block start address)
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //always pointing (real - block start address)
#define HDRP(bp) ((char *)bp - WSIZE) // returns header address
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE) // returns footer address
#define GET_NEXT(bp) ((void *) ((void **)((char *)bp + WSIZE)))
#define GET_PREV(bp) ((void *) ((void **)((char *)bp + 2 * WSIZE)))



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
static void *extend_heap(size_t words);
static void *coalesce(void *bp);




int mm_init(void) {
    /* Initialize an empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    start = heap_listp;

    /* Set up the heap with initial padding, prologue, and epilogue blocks */
    PUT(heap_listp, 0);                            // Alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(2 * WSIZE, 1)); // Prologue header: size=8, allocated=1
    PUT(heap_listp + (2 * WSIZE), PACK(2 * WSIZE, 1)); // Prologue footer: size=8, allocated=1
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // Epilogue header: size=0, allocated=1
    heap_listp += (2 * WSIZE);                     // Position heap pointer after prologue

    /* Extend the heap with a free block to start */
    if (extend_heap(CHUNKSIZE / WSIZE) == (void *) 0)
        return -1;
    return 0;


}


/* bp means "BLOCK POINTER" */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((uint64_t)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); /* Free block header (old epilogue eliminated) */
    PUT(HDRP(bp) + WSIZE, 0);     /* Prev pointer */
    PUT(HDRP(bp) + 2 * WSIZE, 0); /* Next pointer*/
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer (move backward by size), 0 means not allocated!*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}

/* coalesce() is called in case:
    1: extending heap by (sbrk)
    2: free'ing
*/
static void *coalesce(void *bp)
{
    size_t prev_alloc = CHECK(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = CHECK(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

   /*case 1*/





   /*case 2*/





   /*case 3*/







   /*case 4*/

    return bp;
}


/* first -fit finding memory block */

static void *first_fit(size_t request_size){
    char *curr = heap_listp;

    while (GET_SIZE(HDRP(curr)) > 0){
        if (GET_SIZE(HDRP(curr)) >= request_size && (!CHECK(HDRP(curr)))){
            break;
        }
        curr = NEXT_BLKP(curr);
    }

    if (GET_SIZE(HDRP(curr)) <= 0) // cannot find block
        return (void *) 0;
    else
        return (void *) curr;
}


/*  alloc_size would be aligned value */
static void place(void *bp, size_t alloc_size) {
    size_t curr_size = GET_SIZE(HDRP(bp));

    if (curr_size - alloc_size <= WSIZE){ // if request size is close enough to current block size
        PUT(HDRP(bp), PACK(curr_size, 1));
        PUT(FTRP(bp), PACK(curr_size, 1));
        return ;
    }

    PUT(HDRP(bp), PACK(alloc_size, 1));
    PUT(FTRP(bp), PACK(alloc_size, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(curr_size - alloc_size, 0));
    PUT(FTRP(bp), PACK(curr_size - alloc_size, 0));
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size) {
    size_t alloc_size; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        alloc_size = 2 * DSIZE;
    else
        alloc_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);


    /* Search the free list for a fit */
    if ((bp = first_fit(alloc_size)) != NULL) {
        place(bp, alloc_size);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(alloc_size,CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, alloc_size);
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

