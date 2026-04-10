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
#define GET(p)       (*(unsigned int *)(p)) 

// 주소 p가 가리키는 곳에 val 값을 저장해라
#define PUT(p, val)  (*(unsigned int *)(p) = (val))   

// 블록 크기가 16, 할당 여부가 1인 메모리 헤더에 17이 있다면
// 할당 여부를 제외하고 크기만 알고싶을때 사용하는 매크로 
#define GET_SIZE(p)  (GET(p) & ~0x7)

// 반대로 할당 여부만 꺼내는 매크로 
#define GET_ALLOC(p) (GET(p) & 0x1)

// bp를 기준으로 4바이트 앞으로 가면 헤더 주소가 나온다 
#define HDRP(bp)        ((char *)(bp) - WSIZE)

// footer의 시작 지점을 주는 매크로 
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 현재 블록의 다음 블록의 시작 위치(bp)를 구하는 매크로 
#define NEXT_BLKP       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))

// 현재 블록의 이전 블록의 시작 위치(bp)를 구하는 매크로 
#define PREV_BLKP       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
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
    else
    {
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