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
    "team 6",
    /* First member's full name */
    "JewelryKim, Saihim Cho, Sean Huh",
    /* First member's email address */
    "jsjs21good@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Saihim Cho",
    /* Second member's email address (leave blank if none) */
    "saihimcho@kaist.ac.kr",
};

//******MACROs*************//
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */

#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* free list pointers*/
#define PREC(bp) (char *)(bp)
#define SUCC(bp) ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// #define PREV_FREE_BLKP(bp)

static char *heap_listp;
static char *last_freep = 0;
// static char *HEAD = NULL;
static char *FOOT = 0;
static char *heap_listp;

//*******FUNCTIONs********//
int mm_init(void);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *bp, size_t size);
static void remove_block(void *bp);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *tmp;
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */

    heap_listp += (2 * WSIZE);

    // printf("*****mm_init*******\n");
    // printf("\nheap listp: %p\n", heap_listp);
    // printf("PR_header:     %p\n", HDRP(heap_listp));
    // printf("PR_footer:     %p\n", FTRP(heap_listp));

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // if ((tmp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
    if ((last_freep = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
        return -1;
    // last_freep = (char *)tmp;

    // printf("last_freep: %p\n", last_freep);
    // printf("HEADER(last_freep): %d\n", GET(HDRP(last_freep)));

    return 0;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp));
    if (last_freep == 0){
        return bp;
    }
    if (prev_alloc && next_alloc)
    { /* Case 1 */
        if (bp == last_freep){
            return bp;
        }
        else {
            PUT(PREC(bp), last_freep);
            PUT(SUCC(last_freep), (char *) bp);
            PUT(SUCC(bp), NULL);
            last_freep = bp;
            bp = last_freep;
            return bp;
        }
    }
    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        
        remove_block(NEXT_BLKP(bp));

        //내가 last_freep인 경우
        if (NEXT_BLKP(bp) == last_freep)
        {
            PUT(PREC(bp), (void *)GET(PREC(NEXT_BLKP(bp))));

            if (GET(PREC(bp)))
            {
                PUT(SUCC((void *)GET(PREC(bp))), (void *)bp);
            }
        }
        //내가 last_freep가 아닌 경우
        else
        {
            PUT(PREC(bp), last_freep);
            PUT(SUCC(last_freep), bp);
            PUT(SUCC(bp), NULL);
            last_freep = bp;
        }

        bp = last_freep;
    }
    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);


        //가용 블럭 리스트에서 제거 및 연결
        remove_block(bp);

        //내가 last_freep인 경우
        if ( bp == last_freep)
        {
            if (GET(PREC(bp)))
            {
                PUT(SUCC((void *)GET(PREC(bp))), (void *)bp);
            }
        }
        //내가 last_freep가 아닌 경우
        else
        {
            PUT(PREC(bp), last_freep);
            PUT(SUCC(last_freep), bp);
            
            PUT(SUCC(bp), NULL);
            last_freep = bp;
        }

        bp = last_freep;
    }
    else
    { /* Case 4 */
        void *next_bp, *tmp_bp;
        next_bp = NEXT_BLKP(bp);
        tmp_bp = bp;
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        remove_block(bp);
        remove_block(next_bp);
        //내가 last_freep인 경우
        if (next_bp == last_freep){
            void* prev_of_next_bp = (void*)GET(PREC(next_bp));
            PUT(PREC(bp),prev_of_next_bp);
            PUT(SUCC(prev_of_next_bp),bp);
            last_freep = bp;
        }
        else if (bp == last_freep){
            PUT(SUCC(GET(PREC(bp))),bp);
        }
        //내가 last_freep가 아닌 경우
        else{
            PUT(PREC(bp), last_freep);
            PUT(SUCC(last_freep), bp);
            
            PUT(SUCC(bp), NULL);
            last_freep = bp;
        }
    }
    bp = last_freep;
    return bp;
}

//ING
static void remove_block(void *bp)
{
    void *bp_temp = bp;
    void *pre_free, *suc_free;
    pre_free = (void *)GET(PREC(bp));
    suc_free = (void *)GET(SUCC(bp));

    if (pre_free)
    {
        PUT(SUCC(pre_free), suc_free);
    }
    if (suc_free)
    {
        PUT(PREC(suc_free), pre_free);
    }
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(PREC(bp), last_freep);            /* Free block footer */
    PUT(SUCC(bp), FOOT);                  /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    //return coalesce(bp); //original code
    //debug code
    char *temp_bp = coalesce(bp);
    // printf("***extned_heap returns bp: %p***\n", temp_bp);
    return temp_bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    // printf("\n*****mm_malloc*******\n");

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // printf("size:        %d\n", size);
    // printf("asize:       %d\n", asize);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;
    for (bp = last_freep; bp != 0; bp = (void *)GET(PREC(bp)))
    {
        if ((asize <= GET_SIZE(HDRP(bp)))) // !GET_ALLOC(HDRP(bp)) &&
        {
            return bp;
        }
    }
    return NULL; /* No fit */
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    // free list의 bp를 기준으로 그 bp의 pred와 succ존재여부를 탐색.
    //bp의 pred의 succ을 bp의 succ으로 연결. 그 역도 마찬가지.

    printf("\n----IN PLACE-----\n");
    printf("\n*****BEFORE*******\n");
    printf("Header:      %p\n", HDRP(bp));
    printf("Footer:      %p\n", FTRP(bp));
    printf("Predecessor: %p [%p] \n", PREC(bp), GET(PREC(bp)));
    printf("successor:   %p [%p] \n", SUCC(bp), GET(SUCC(bp)));

    void *bp_temp = bp;
    void *pre_free, *suc_free;
    pre_free = (void *)GET(PREC(bp));
    suc_free = (void *)GET(SUCC(bp));

    if (pre_free)
    {
        PUT(SUCC(pre_free), suc_free);
    }
    if (suc_free)
    {
        PUT(PREC(suc_free), pre_free);
    }

    if ((csize - asize) >= (2 * DSIZE))
    {

        PUT(HDRP(bp), PACK(asize, 1)); // heap list
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));

        if (bp_temp == last_freep)
        {
            PUT(PREC(bp), NULL);
            PUT(SUCC(bp), NULL);
        }
        else
        {
            PUT(PREC(bp), last_freep);
            PUT(SUCC(bp), NULL);
            PUT(SUCC(last_freep), bp);
        }

        last_freep = bp;
    }
    else // 갖고 온 블록만 딱 넣을 수 있는 상황. free block이 나올수 없음.
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        if (pre_free == NULL && suc_free == NULL)
        {
            last_freep = NULL;
        }
        else if (pre_free && !(suc_free))
        {
            last_freep = (void *)GET(PREC(bp));
        }
    }

    // printf("*****AFTER*******\n");
    // printf("Header:      %p\n", HDRP(bp));
    // printf("Footer:      %p\n", FTRP(bp));
    // printf("Predecessor: %p [%p] \n", PREC(bp), GET(PREC(bp)));
    // printf("successor:   %p [%p] \n", SUCC(bp), GET(SUCC(bp)));
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *oldbp = bp;
    void *newbp;
    size_t copySize;

    newbp = mm_malloc(size);
    if (newbp == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(bp));
    if (size < copySize)
        copySize = size;
    memcpy(newbp, oldbp, copySize);
    mm_free(oldbp);
    return newbp;
}
