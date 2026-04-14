
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

// 가용 리스트 조작 매크로 

// 4바이트를 한 묶음(word)으로 보겠다 주소로 하면 4개(1000, 1001, 1002, 1003)
#define WSIZE      4

// 8바이트를 한 묶음(word)으로 보겠다 주소로 하면 8개 (1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007)
#define DSIZE      8 

// 힙(Heap) 메모리가 부족할 때, 운영체제로부터 한 번에 받아오는 메모리의 기본 단위 (보통 1개 페이지 크기)
#define CHUNKSIZE (1<<12) 

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // MAX(x, y)는 두 값을 비교해서 더 큰 값을 반환하는 매크로 

// PACK(8, 1): 여기서부터 8바이트는 사용 중인 칸이야
// PACK(16, 0): 여기서부터 16바이트는 비어 있는 칸이야 
#define PACK(size, alloc) ((size) | (alloc))

// 주소 p가 가리키는 곳에 저장된 4바이트 값을 unsigned int로 읽어와라
#define GET(p)      (*(unsigned int *)(p))

// 주소 p가 가리키는 곳에 val 값을 저장해라
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 블록 크기가 16, 할당 여부가 1인 메모리 헤더에 17이 있다면
// 할당 여부를 제외하고 크기만 알고싶을때 사용하는 매크로 
#define GET_SIZE(p)  (GET(p) & ~0x7)

// 반대로 할당 여부만 꺼내는 매크로 
#define GET_ALLOC(p) (GET(p) & 0x1)

// bp를 기준으로 4바이트 앞으로 가면 헤더 주소가 나온다 
#define HDRP(bp)           ((char *)(bp) - WSIZE)

// footer의 시작 지점을 주는 매크로 
#define FTRP(bp)           ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 현재 블록의 다음 블록의 시작 위치(bp)를 구하는 매크로 
#define NEXT_BLKP(bp)      ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))

// 현재 블록의 이전 블록의 시작 위치(bp)를 구하는 매크로 
#define PREV_BLKP(bp)      ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// 전역 변수
static char *heap_listp;
static char *next_fit_ptr; // next_fit 구현을 위한 포인터 

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        /* nothing */
    }
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // rover가 병합된 블록 내부를 가리키면 시작점으로 조정 
    if (next_fit_ptr >= (char *)bp && next_fit_ptr < NEXT_BLKP(bp)) {
        next_fit_ptr = bp;
    }

    return bp;
}


static void *extend_heap(size_t words)
{
    char *bp; 
    size_t size; 

    if (words % 2 == 0)
    {
        // words가 짝수일때
        size = words * WSIZE; 
    }
    else 
    {
        // words가 홀수일때 
        size = (words + 1) * WSIZE; 
    }

    if ((long) (bp = mem_sbrk(size)) == -1) 
    {
        return NULL; 
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); 

    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    next_fit_ptr = NEXT_BLKP(heap_listp);
    return 0;
}

static void *find_fit(size_t asize)
{
    char *oldptr;

    if (next_fit_ptr == NULL)
        next_fit_ptr = heap_listp;

    oldptr = next_fit_ptr;

    // 1차 탐색: next_fit_ptr부터 epilogue 전까지 
    // epilogue는 size가 0이기 때문에 거기에서 멈춤 
    while (GET_SIZE(HDRP(next_fit_ptr)) > 0) {
        if (!GET_ALLOC(HDRP(next_fit_ptr)) &&
            asize <= GET_SIZE(HDRP(next_fit_ptr))) {
            return next_fit_ptr;
        }
        next_fit_ptr = NEXT_BLKP(next_fit_ptr);
    }

    /* 2차 탐색: heap 시작부터 원래 시작점 직전까지 */
    next_fit_ptr = heap_listp;
    while (next_fit_ptr < oldptr) {
        if (!GET_ALLOC(HDRP(next_fit_ptr)) &&
            asize <= GET_SIZE(HDRP(next_fit_ptr))) {
            return next_fit_ptr;
        }
        next_fit_ptr = NEXT_BLKP(next_fit_ptr);
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        void *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(next_bp), PACK(csize - asize, 0));
        PUT(FTRP(next_bp), PACK(csize - asize, 0));

        /* 다음 탐색은 방금 생긴 free block부터 */
        next_fit_ptr = next_bp;
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        /* 다음 탐색은 그 다음 블록부터 */
        next_fit_ptr = NEXT_BLKP(bp);
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; 
    size_t extendsize; 
    char *bp; 

    if (size == 0)
    {
        return NULL; 
    }

    if (size <= DSIZE)
    {
        asize = 2 * DSIZE; 
    }
    else 
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE -1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL) 
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, asize);
    return bp; 
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *oldptr = bp;
    void *newptr;
    size_t oldSize; //현재블록 사이즈
    size_t asize;   //실제 할당할 사이즈
    size_t copySize;    //현재 블록의 payload 사이즈 
    size_t nextSize;
    size_t totalSize;

    if (bp == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(bp);
        return NULL;
    }

    oldSize = GET_SIZE(HDRP(oldptr));
    if(size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    copySize = oldSize - DSIZE;
    

    //헤더, 풋터 제외하고 데이터 만큼만 기존 크기 가져옴
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    //수정하기

    void *nextbp = NEXT_BLKP(oldptr);
    //만약 다음 블록이 free이고 블록 크기가 충분할때
    nextSize = GET_SIZE(HDRP(nextbp));
    totalSize = oldSize + nextSize;
    if (asize <= totalSize && !GET_ALLOC(HDRP(nextbp))){
        //확장하고 남는 공간은 free 블록으로
        if ((totalSize - asize) >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            nextbp = NEXT_BLKP(bp);
            PUT(HDRP(nextbp), PACK(totalSize - asize, 0));
            PUT(FTRP(nextbp), PACK(totalSize - asize, 0));
        }
        else {
            PUT(HDRP(bp), PACK(totalSize, 1));
            PUT(FTRP(bp), PACK(totalSize, 1));
        }
        return oldptr;
    }
    //그렇지 않을 경우
    newptr = mm_malloc(size);   //사이즈만큼 할당
    if (newptr == NULL) //만약 새로운 포인터가 할당 안될시에
        return NULL;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}