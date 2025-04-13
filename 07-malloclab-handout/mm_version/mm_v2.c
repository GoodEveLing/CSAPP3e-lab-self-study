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

static char* heap_listp;
static char* rover;

/* Function declaration */
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void  place(void* bp, size_t asize);

static void checkblock(void* bp);
static void checkheap(int verbose);
static void printblock(void* bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* creater the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1) return -1;
    PUT(heap_listp, 0);                            /* Alignment padding 4B */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header 序言头 8/1*/
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer 序言尾 8/1 */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header 结尾头表示堆的结束 0/1 */

    /* 第一个字是不使用的填充字，正常的堆块在序言块和结尾块之间。
       序言块 = header(4B) + footer(4B) = 8B, 设置chunk size = DSIZE是头尾相加的总值
       结尾块是一个已分配的0B的块，当后继块大小为0，那么便可判断其达到了结尾。
        */
    /* 对齐到起始块的有效载荷，此时heap_listp指向Prologue footer */

    /*
    起始堆结构：
    | Alignment padding (4B) | Prologue header (4B) | Prologue footer (4B) | Epilogue header (4B) |
                                                    ^ heap_listp
    */
    heap_listp += (2 * WSIZE);
    rover = heap_listp;
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
    /* 如果words是奇数，则将其加1变成偶数，如果是偶数，则不变。这样确保分配的块是8字节（2*
     * WSIZE）对齐的 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    /* Initialize free block header/footer and the epilogue header */
    /* HDRP(bp) = bp - WSIZE = bp - 4B, 是不是覆盖了前面的4B，前面的是结尾块？*/
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp); /* why？ 未扩展前的堆可能以空闲块结尾，合并这两个空闲块，并返回块指针bp*/
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
        asize = DSIZE * DIV_ROUND_UP(size + DSIZE, DSIZE); /* why? */

    /* Search the free list for a fit */
    bp = find_fit(asize);

    if (bp != NULL) {
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
    char*  ptr   = rover;
    size_t alloc = GET_ALLOC(HDRP(ptr));
    size_t size  = GET_SIZE(HDRP(ptr));

    while (size > 0) {
        if ((size >= asize) && (alloc == 0)) {
            rover = NEXT_BLKP(ptr);
            return ptr;
        }
        ptr   = NEXT_BLKP(ptr);
        alloc = GET_ALLOC(HDRP(ptr));
        size  = GET_SIZE(HDRP(ptr));
    }


    ptr   = heap_listp;
    alloc = GET_ALLOC(HDRP(ptr));
    size  = GET_SIZE(HDRP(ptr));

    while (size > 0) {
        if ((size >= asize) && (alloc == 0)) {
            rover = NEXT_BLKP(ptr);
            return ptr;
        }
        ptr   = NEXT_BLKP(ptr);
        alloc = GET_ALLOC(HDRP(ptr));
        size  = GET_SIZE(HDRP(ptr));
    }

    return NULL;
}

/* 如果剩余部分的长度大于等于2*
 * DSIZE（即最小块大小），则将剩余部分分割成两个块；否则将整个块设置为已分配 */
static void place(void* bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));

    if (size < asize) return;

    /* asize已经是8字节对齐的了， 所以size - asize也是8字节对齐的 */
    if (size - asize >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size - asize, 0));
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
    size_t pre_size    = GET_SIZE(HDRP(pre_ptr));
    size_t next_size   = GET_SIZE(HDRP(next_ptr));

    if (pre_isfree == 1 && next_isfree == 1) {
        return bp;
    }
    else if (pre_isfree == 0 && next_isfree == 1) {
        size += pre_size;
        PUT(HDRP(pre_ptr), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = pre_ptr;
    }
    else if (pre_isfree == 1 && next_isfree == 0) {
        size += next_size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0)); /* 头改变后，对应footer位置也变化了 */
    }
    else if (pre_isfree == 0 && next_isfree == 0) {
        size += next_size + pre_size;
        PUT(HDRP(pre_ptr), PACK(size, 0));
        PUT(FTRP(next_ptr), PACK(size, 0));
        bp = pre_ptr;
    }

    if (rover > bp && rover < NEXT_BLKP(bp)) rover = bp;

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

    newptr          = mm_malloc(size);
    size_t copysize = MIN(size, old_size);
    memcpy(newptr, ptr, copysize);
    mm_free(ptr);

    return newptr;
}

/**************************** check functions ***************************** */

static void checkblock(void* bp)
{
    if ((size_t)bp % 8) printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp))) printf("Error: header does not match footer\n");
}

/*
 * checkheap - Minimal check of the heap for consistency
 */
static void checkheap(int verbose)
{
    char* bp = heap_listp;

    if (verbose) printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) printblock(bp);
        checkblock(bp);
    }

    if (verbose) printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) printf("Bad epilogue header\n");
}

static void printblock(void* bp)
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize  = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize  = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("[%p: header: [%ld:%c] footer: [%ld:%c]\n",
           bp,
           hsize,
           (halloc ? 'a' : 'f'),
           fsize,
           (falloc ? 'a' : 'f'));
}
