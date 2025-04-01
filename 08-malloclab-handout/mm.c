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

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constant and macros */
#define WSIZE 4             /* word and header/footer size (bytes)*/
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* extend heap by this amout (bytes)*/

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and alloccated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from address p */
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

static char* heap_listp;

/* Function declaration */
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void  place(void* bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* creater the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1) return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header 序言头*/
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer 序言尾*/
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header 结尾header */
    heap_listp += (2 * WSIZE);                     /* why？ */

    /* Extend the empty head with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
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
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

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
        asize = 2 * DSIZE; /* 强制要求最小块的大小 = 16B = 4B header + 4B footer + 8B(padding) +
                              0Bpayload */
    else
        asize = DSIZE * DIV_ROUND_UP(size, DSIZE);   // asize是包含头尾的数据块

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
    char*  ptr   = heap_listp;
    size_t alloc = GET_ALLOC(HDRP(ptr));
    size_t size  = GET_SIZE(HDRP(ptr));

    while (size > 0) {
        if ((size >= asize) && (alloc == 0)) {
            return ptr;
        }
        ptr   = NEXT_BLKP(ptr);
        alloc = GET_ALLOC(HDRP(ptr));
        size  = GET_SIZE(HDRP(ptr));
    }

    return NULL;
}

static void place(void* bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));

    if (size < asize) return;

    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));

    if (size - asize >= 2 * DSIZE) {
        size_t remain_size = DSIZE * ((size - asize) / DSIZE);
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
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
    /* 合并空闲块 */
    coalesce(ptr);
}

static void* coalesce(void* bp)
{
    char*  pre_ptr     = PREV_BLKP(bp);
    char*  next_ptr    = NEXT_BLKP(bp);
    char   pre_isfree  = GET_ALLOC(FTRP(pre_ptr));
    char   next_isfree = GET_ALLOC(HDRP(next_ptr));
    size_t size        = GET_SIZE(HDRP(bp));
    size_t pre_size    = GET_SIZE(bp - DSIZE);
    size_t next_size   = GET_SIZE(bp + size - WSIZE);

    if (pre_isfree == 1 && next_isfree == 1) {
        return bp;
    }
    else if (pre_isfree == 0 && next_isfree == 1) {
        PUT(HDRP(pre_ptr), PACK(size + pre_size, 0));
        PUT(FTRP(bp), PACK(size + pre_size, 0));
        bp = pre_ptr;
    }
    else if (pre_isfree == 1 && next_isfree == 0) {
        PUT(HDRP(bp), PACK(size + next_size, 0));
        PUT(FTRP(next_ptr), PACK(size + next_size, 0));
    }
    else if (pre_isfree == 0 && next_isfree == 0) {
        PUT(HDRP(pre_ptr), PACK(size + next_size + pre_size, 0));
        PUT(FTRP(next_ptr), PACK(size + pre_size + pre_size, 0));
        bp = pre_ptr;
    }

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

    if (ptr == NULL) {
        return mm_malloc(size);
    }

    size_t old_size = GET_SIZE(HDRP(ptr));

    if (size == old_size) {
        return ptr;
    }

    newptr = mm_malloc(size);
    if (newptr != NULL) {
        memcpy(newptr, ptr, old_size);
        mm_free(ptr);
    }
    else {
        extend_heap(size / DSIZE);
        newptr = mm_malloc(size);
        memcpy(newptr, ptr, old_size);
        mm_free(ptr);
    }
    return newptr;
}
