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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constant and macros */
#define WSIZE 4             /* word and header/footer size (bytes)*/
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* extend heap by this amout (bytes)*/

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Pack a size and alloccated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from address p, p is header oor footer */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next adn previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

/* n除以d 向上取整 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) ((DSIZE) * (((size) + (DSIZE) + (DSIZE - 1)) / (DSIZE)))

static char* heap_listp = 0;

#define LIST_NUM 10
static char* free_listp = 0;

/*
free bloc = header + pred + succ + payload + footer
*/
/* Given block ptr bp, compute value of its succ and pred */
#define GET_SUCC(bp) (*(unsigned int*)(bp))
#define GET_PRED(bp) (*((unsigned int*)(bp) + 1))

/* Put a value at address of succ and pred */
#define PUT_SUCC(bp, val) (GET_SUCC(bp) = (unsigned int)(val))
#define PUT_PRED(bp, val) (GET_PRED(bp) = (unsigned int)(val))

/* Function declaration */
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void  place(void* bp, size_t asize);

int get_index(size_t size)
{
    if (size <= 16) {
        return 0;
    }
    else if (size <= 32) {
        return 1;
    }
    else if (size <= 64) {
        return 2;
    }
    else if (size <= 128) {
        return 3;
    }
    else if (size <= 256) {
        return 4;
    }
    else if (size <= 512) {
        return 5;
    }
    else if (size <= 1024) {
        return 6;
    }
    else if (size <= 2048) {
        return 7;
    }
    else if (size <= 4096) {
        return 8;
    }
    else {
        return 9;
    }
}
void* get_free_list(size_t size)
{
    // int i = 0;

    // /* i > 4096 */
    // if (size >= 4096) return free_listp + (9 * WSIZE);

    // /* i <= 16 */
    // size = size >> 4;

    // /* other */
    // while (size) {
    //     size = size >> 1;
    //     i++;
    // }

    int i = get_index(size);

    return (char*)free_listp + (i * WSIZE);
}

static void insert_free_block(void* bp)
{
    if (bp == NULL) return;
    void* root = get_free_list(GET_SIZE(HDRP(bp)));
    void* pred = root;
    void* succ = GET(root);

    /* size form small to big to aviod always use big free block */
    while (succ != NULL) {
        if (GET_SIZE(HDRP(succ)) >= GET_SIZE(HDRP(bp))) break;
        pred = succ;
        succ = GET_SUCC(succ);
    }

    /* Now, SIZE: pred < bp < succ */
    if (pred == NULL && succ == NULL) {
        /* NULL -> bp -> NULL */
        root = bp;
    }
    else if (pred != NULL && succ == NULL) {
        /* pred -> bp -> NULL */
        PUT_SUCC(pred, bp);
        PUT_PRED(bp, pred);
        PUT_SUCC(bp, NULL);
    }
    else if (pred == NULL && succ != NULL) {
        /* NULL->bp->succ */
        PUT_SUCC(bp, succ);
        PUT_PRED(succ, bp);
        PUT_PRED(bp, NULL);
    }
    else {
        PUT_PRED(bp, pred);
        PUT_SUCC(bp, succ);

        PUT_PRED(succ, bp);
        PUT_SUCC(pred, bp);
    }
}

static void delete_alloc_block(void* bp)
{
    void* root = get_free_list(GET_SIZE(HDRP(bp)));
    void* pred = GET_PRED(bp);
    void* succ = GET_SUCC(bp);

    PUT_PRED(bp, NULL);
    PUT_SUCC(bp, NULL);

    if (pred == NULL) {
        /* NULL-> bp ->NULL, bp is root */
        PUT_SUCC(root, succ);

        /* NULL-> bp ->FREE_BLK */
        if (succ != NULL) {
            PUT_PRED(succ, NULL);
            root = succ;
        }
    }
    else {
        /* FREE_BLK-> bp ->NULL, pred or pred node is root */
        PUT_SUCC(pred, succ);

        /* FREE_BLK-> bp ->FREE_BLK */
        if (succ != NULL) PUT_PRED(succ, pred);
    }
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* creater the initial empty heap */
    if ((heap_listp = mem_sbrk(14 * WSIZE)) == (void*)-1) return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), 0);
    PUT(heap_listp + (2 * WSIZE), 0);
    PUT(heap_listp + (3 * WSIZE), 0);
    PUT(heap_listp + (4 * WSIZE), 0);
    PUT(heap_listp + (5 * WSIZE), 0);
    PUT(heap_listp + (6 * WSIZE), 0);
    PUT(heap_listp + (7 * WSIZE), 0);
    PUT(heap_listp + (8 * WSIZE), 0);
    PUT(heap_listp + (9 * WSIZE), 0);
    PUT(heap_listp + (10 * WSIZE), 0); /* padding */

    PUT(heap_listp + (11 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (12 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (13 * WSIZE), PACK(0, 1));     /* Epilogue header */

    free_listp = heap_listp;    /* pointer to block size <= 32 */
    heap_listp += (12 * WSIZE); /* pointer to Prologue footer */

    /* Extend the empty head with a free block of CHUNKSIZE bytes */
    if (extend_heap(2 * DSIZE / WSIZE) == NULL) return -1;
    return 0;
}

/*
 * extend heap
 */
static void* extend_heap(size_t words)
{
    char*  bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    /* Initialize free block header/footer and the epilogue header */
    /* HDRP(bp) = bp - WSIZE = bp - 4B, 是不是覆盖了前面的4B，前面的是结尾块？*/
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Initialize explicit free list ptr: efree_listp, succ and pred */
    PUT_SUCC(bp, 0); /* Free block succ */
    PUT_PRED(bp, 0); /* Free block pred */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char*  bp;

    /* Ignore spurious requests */
    if (size == 0) return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = ALIGN(size);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}


static void* find_fit(size_t asize)
{
    for (void* root = get_free_list(asize); root != (heap_listp - WSIZE); root += WSIZE) {
        void* bp = GET(root);
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize) return bp;
            bp = GET_SUCC(bp);
        }
    }

    return NULL; /* No fit */
}

/* 如果剩余部分的长度大于等于2*
 * DSIZE（即最小块大小），则将剩余部分分割成两个块；否则将整个块设置为已分配 */
static void place(void* bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));

    if (size < asize) return;

    delete_alloc_block(bp);

    /* asize已经是8字节对齐的了， 所以size - asize也是8字节对齐的 */
    if (size - asize >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
        insert_free_block(bp);
    }
    else {
        /* there must use size but not asize, for alignment */
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}

/*
 * mm_free
 */
void mm_free(void* ptr)
{
    if (ptr == NULL) return;

    size_t size = GET_SIZE(HDRP(ptr));

    /* 设置这个块的状态为空闲*/
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    PUT_PRED(ptr, NULL);
    PUT_SUCC(ptr, NULL);
    /* 合并空闲块 */
    coalesce(ptr);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size       = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* alloc-> bp ->alloc */
                                    /* then we will insert(bp) */
    }

    else if (prev_alloc && !next_alloc) { /* alloc-> bp ->free */
        delete_alloc_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* free-> bp ->alloc */
        delete_alloc_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); /* prev block as first free block */
    }

    else { /* free-> bp ->free */
        delete_alloc_block(NEXT_BLKP(bp));
        delete_alloc_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_free_block(bp);

    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{
    void* newptr = ptr;

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (ptr == NULL) return mm_malloc(size);

    size_t old_size = GET_SIZE(HDRP(ptr));

    if (size == old_size) return ptr;

    newptr = mm_malloc(size);
    if (!newptr) return NULL;

    size_t copysize = MIN(size, old_size);
    memcpy(newptr, ptr, copysize);
    mm_free(ptr);

    return newptr;
}
