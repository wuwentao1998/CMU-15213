/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reus00ed. Realloc is
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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
///////////////////////////////////////////////
#define WSIZE     4
#define DSIZE     8
#define CHUNKSIZE (1 << 12)

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(size_t*)(p))

#define PUT(p, val) (*(size_t*)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)

#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp) - WSIZE)

#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)))
 
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

//////////////////////////////////////////////
static char* head_listp;

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);

/////////////////////////////////////////////
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

	if ((head_listp = mem_sbrk(4 * WSIZE)) == -1)
		return -1;

	PUT(head_listp, 0);
	PUT(head_listp + (1 * WSIZE), PACK(DSIZE, 1));
	PUT(head_listp + (2 * WSIZE), PACK(DSIZE, 1));
	PUT(head_listp + (3 * WSIZE), PACK(0, 1));
	
	if (extend_heap(CHUNKSIZE) == NULL)
		return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	if (size == 0)
		return NULL;
	if (size <= DSIZE)
		size = 2 * DSIZE;
	else
		size = ALIGN(size + DSIZE);

	void* new_bp;
	if ((new_bp = find_fit(size)) == NULL)
	{
		if ((new_bp = extend_heap(size)) == NULL)
			return NULL;
		else
			place(new_bp, size);
	}
	else
		place(new_bp, size);

	return new_bp;
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
void *mm_realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
	{
		void* ptr = mm_malloc(size);
		return ptr;
	}
	else if (size == 0)
	{
		mm_free(ptr);
		return ptr;
	}
	else
	{
		size_t old_size = GET_SIZE(HDRP(ptr));

		if (old_size >= size)
			return ptr;
		else
		{
			size_t csize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
			int isEmpty = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

			if (csize >= size - old_size && isEmpty == 0)
			{
				PUT(HDRP(ptr), PACK((old_size + csize), 1));
				PUT(FTRP(ptr), PACK((old_size + csize), 1));
				return ptr;
			}
			else
			{
				void* new_block = mm_malloc(size);
				memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
				mm_free(ptr);
				return new_block;
			}
		}
	}
}

/////////////////////////////////////////
static void* extend_heap(size_t words)
{
	void* new_bp;
	words = ALIGN(words);

	if ((new_bp = mem_sbrk(4 * WSIZE)) == -1)
		return NULL;

	PUT(HDRP(new_bp), PACK(words, 0));//覆盖了原来的结尾符
	PUT(FTRP(new_bp), PACK(words, 0));
	PUT(HDRP(NEXT_BLKP(new_bp)), PACK(0, 1));
	return coalesce(new_bp);
}



static void* coalesce(void* bp)
{
	void* prev = PREV_BLKP(bp);
	void* next = NEXT_BLKP(bp);
	size_t size = GET_SIZE(HDRP(bp));

	if (GET_ALLOC(HDRP(prev)) && GET_ALLOC(HDRP(next)))
		return bp;

	if (GET_ALLOC(HDRP(prev)) && !GET_ALLOC(HDRP(next)))
	{
		size += GET_SIZE(HDRP(next));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		//注意此时size已经更新过了，所以使用bp也可,同时next的size并未修改所以使用next也可
	}

	if (!GET_ALLOC(HDRP(prev)) && GET_ALLOC(HDRP(next)))
	{
		size += GET_SIZE(HDRP(prev));
		PUT(HDRP(prev), PACK(size, 0));
		PUT(FTRP(prev), PACK(size, 0));
		bp = prev;
	}

	if (!GET_ALLOC(HDRP(prev)) && !GET_ALLOC(HDRP(next)))
	{
		size += GET_SIZE(HDRP(prev)) + GET_SIZE(HDRP(next));
		PUT(HDRP(prev), PACK(size, 0));
		PUT(FTRP(next), PACK(size, 0));
		bp = prev;
	}

	return bp;
}


static void* find_fit(size_t asize)
{
	for (void* bp = head_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_BLKP(bp))
	{
		if (GET_SIZE(HDRP(bp)) > asize)
			return bp;
	}

	return NULL;
}


static void place(void* bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
	size_t bsize = csize - asize;

	if (bsize >= 2 * DSIZE)
	{
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(bsize, 0));
		PUT(FTRP(bp), PACK(bsize, 0));
	}
	else
	{
		PUT(HDRP(bp), PACK(csize, 1));//注意是csize不是asize
		PUT(FTRP(bp), PACK(csize, 1));
	}
} 



