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

#define RIGHT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //always pointing (real - block start address)
#define LEFT_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //always pointing (real - block start address)
#define HDRP(bp) ((char *)bp - WSIZE) // returns header address
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE) // returns footer address

#define PREV_BP(bp) (void *) (*(uint64_t *)(bp))
#define NEXT_BP(bp) (void *) (*(uint64_t *)((char *)bp + 1 * WSIZE))

#define SET_NEXT(bp, addr) PUT((char *) bp + WSIZE, (uint64_t )addr)
#define SET_PREV(bp, addr) PUT(bp, (uint64_t )addr)





/* global variables */

/* heap_listp will be the root node */
static char *heap_listp = 0;


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
/*
    Examples :
    ALIGN(7) -> 8
    ALIGN(6) -> 8
*/

/*
    size_t -> system's maximum data size type
    SIZE_T_SIZE aligned size of size_t
*/

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void remove_from_list(void *bp);
static void add_to_list(void *bp);


int mm_init(void) {
    /* Initialize an empty heap */
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1)
        return -1;

    /* Set up the heap with initial padding, prologue, and epilogue blocks */
    PUT(heap_listp, 0);                            // Alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(4 * WSIZE, 1)); // Prologue header: size= 32, allocated=1
    PUT(heap_listp + (2 * WSIZE), 0);                  // Prev pointer
    PUT(heap_listp + (3 * WSIZE), 0);                  // Next pointer
    PUT(heap_listp + (4 * WSIZE), PACK(4 * WSIZE, 1)); // Prologue footer: size= 32, allocated=1
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));     // Epilogue header: size=0, allocated=1
    heap_listp += (2 * WSIZE);                     // Pointing Next area

    /* Extend the heap with a free block to start */
    if (extend_heap(CHUNKSIZE / WSIZE) == (void *) 0)
        return -1;
    return 0;

    /*

    Initial Heap diagram
    0          8              16              24              32              40              48
    +----------+---------------+---------------+---------------+---------------+---------------+---------------+
    | padding  | prologue hdr  |     prev      |     next      | prologue ftr  |   epilogue    |               |
    |          |     32/1      |      0        |       0       |     32/1      |     0/1       |               |
    +----------+---------------+---------------+---------------+---------------+---------------+---------------+
                               |                                                               |
                               |                                                               |
                               |                                                               |
                            heap_list_p                                                       brk
    */
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

    PUT(HDRP(bp), PACK(size, 0)); /* New free block header (old epilogue header eliminated) */
    PUT(HDRP(bp) + WSIZE, 0);     /* Prev pointer sets to NULL */
    PUT(HDRP(bp) + 2 * WSIZE, 0); /* Next pointer sets to NULL */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer (move backward by size), 0 means not allocated!*/
    PUT(HDRP(RIGHT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}

/*


Initial heap:
+-----------------------------------------------------------+
| Address | Content                                          |
+---------+--------------------------------------------------+
| 0       | padding (0/0)                                    |
| 8       | prologue header (32/1)                           |
| 16      | prev (0/0)                                       |
| 24      | next (0/0)                                       |
| 32      | prologue footer (32/1)                           |
| 40      | epilogue header (0/1)                            |
+---------+--------------------------------------------------+



After extend_heap(4096):
+-----------------------------------------------------------------------------------------------------------------------+
| Address | Content                                                                                                    |
+---------+------------------------------------------------------------------------------------------------------------+
| 0       | padding (0/0)                                                                                              |
| 8       | prologue header (32/1)                                                                                     |
| 16      | prev (0/0) - Pointer to the previous free block, here it's NULL because prologue block can't be free       |
| 24      | next (0/0) - Pointer to the next free block, initially NULL as there is no free block following prologue   |
| 32      | prologue footer (32/1) - Same as prologue header, marks end of the prologue block                          |
| 40      | new free block header (4096/0) - Replaces old epilogue header                                              |
| 48      | prev (0/0) - Pointer to the previous free block, NULL in this case                                         |
| 56      | next (0/0) - Pointer to the next free block, NULL as this is the only free block                           |
| ...     | payload (free space for user allocation) ...                                                               |
| 4136    | new free block footer (4096/0) - Footer of the new free block                                              |
| 4144    | epilogue header (0/1) - Marks the end of the heap, size 0 and allocated status                             |
+---------+------------------------------------------------------------------------------------------------------------+

*/


/* remove free block pointer from linked list */
static void remove_from_list(void *bp)
{
    void *prev_bp = PREV_BP(bp);
    void *next_bp = NEXT_BP(bp);

    /* if we remove not last block */
    if (next_bp != NULL){
        SET_NEXT(prev_bp, next_bp);
        SET_PREV(next_bp, prev_bp);
    }
    /* if we remove last block */
    else{
        SET_NEXT(prev_bp , NULL);
        SET_PREV(bp, NULL);
    }
}

/* add free block pointer to linked list in first position */
/* LIFO Implementation */
static void add_to_list(void *bp)
{
    void *original_first_elem = NEXT_BP(heap_listp);

    SET_NEXT(bp, original_first_elem);
    SET_PREV(bp, heap_listp);

    if (original_first_elem != NULL){
        SET_PREV(original_first_elem, bp);
    }
    SET_NEXT(heap_listp, bp);
}


/* coalesce() is called in case:
    1: extending heap by (sbrk)
    2: free'ing
*/


static void *coalesce(void *bp)
{
    size_t left_alloc = CHECK(FTRP(LEFT_BLKP(bp)));
    size_t right_alloc = CHECK(HDRP(RIGHT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (left_alloc && !right_alloc) { /* Case 2 - bp's next block is unallocated */
        remove_from_list(RIGHT_BLKP(bp));
        size += GET_SIZE(HDRP(RIGHT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); // Header size will be updated
        PUT(FTRP(bp), PACK(size, 0)); // New Footer size will be updated
    }

    else if (!left_alloc && right_alloc) { /* Case 3 */
        printf("Case 3 occurred!\n");
        remove_from_list(LEFT_BLKP(bp));
        size += GET_SIZE(HDRP(LEFT_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0)); //Footer size will be updated
        PUT(HDRP(LEFT_BLKP(bp)), PACK(size, 0)); // New footer size will be updated
        bp = LEFT_BLKP(bp);
    }

    else { /* Case 4 */
        printf("Case 4 occurred!\n");
        remove_from_list(LEFT_BLKP(bp));
        remove_from_list(RIGHT_BLKP(bp));
        size += GET_SIZE(HDRP(LEFT_BLKP(bp))) + GET_SIZE(FTRP(RIGHT_BLKP(bp)));
        PUT(HDRP(LEFT_BLKP(bp)), PACK(size, 0)); // New header size will be updated
        PUT(FTRP(RIGHT_BLKP(bp)), PACK(size, 0)); // New footer size will be updated
        bp = LEFT_BLKP(bp);
    }

    add_to_list(bp);

    return bp;
}

/* first -fit finding memory block */
/* traversal through pointers*/

static void *first_fit(size_t request_size){
    void *curr = NEXT_BP(heap_listp);

    while (CHECK(curr) != 0){
        printf("Searching...\n");
        if (GET_SIZE(HDRP(curr)) >= request_size){
            break;
        }
        printf("curr :%p", curr);
        curr = NEXT_BP(curr);
    }

    if (GET_SIZE(HDRP(curr)) < request_size) // cannot find block
        return (void *) 0;
    else
        return (void *) curr;
}


/*  alloc_size would be aligned value */
static void place(void *bp, size_t alloc_size) {
    size_t curr_size = GET_SIZE(HDRP(bp));
    remove_from_list(bp);

    if (curr_size - alloc_size <= WSIZE){ // if request size is close enough to current block size
        PUT(HDRP(bp), PACK(curr_size, 1));
        PUT(FTRP(bp), PACK(curr_size, 1));
        return ;
    }
    PUT(HDRP(bp), PACK(alloc_size, 1));
    PUT(FTRP(bp), PACK(alloc_size, 1));
    bp = RIGHT_BLKP(bp);
    printf("split, NEXT bp hdr : %p\n", HDRP(bp));
    PUT(HDRP(bp), PACK(curr_size - alloc_size, 0));
    PUT(FTRP(bp), PACK(curr_size - alloc_size, 0));
    SET_NEXT(bp, 0);
    SET_PREV(bp, 0);
    add_to_list(bp);
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
    printf("No fit found \n");
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
    printf("free'ing ...\n");
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    remove_from_list(bp);
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

