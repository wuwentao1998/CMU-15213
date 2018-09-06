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
	"XXXXXXX",
	/* First member's full name */
	"WWT",
	/* First member's email address */
	"@SJTU",
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

#define WSIZE 4
#define DSIZE 8           /*Double word size*/
#define CHUNKSIZE (1<<12) /*the page size in bytes is 4K*/
#define MAXLIST 10

//////////////////////////////////////////
#define MAX(x,y)    ((x)>(y)?(x):(y))

#define PACK(size,alloc)    ((size) | (alloc))

#define GET(p)  (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)    ((char *)(bp)-WSIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define SET_BP(p, bp) (*(unsigned int *)(p) = (unsigned int)(bp))
#define PRED_BP(bp) ((char *)(bp))
#define SUCC_BP(bp) ((char *)(bp) + WSIZE)
#define PRED(bp) (*(char **)(bp)) //为什么是char呢？
#define SUCC(bp) (*(char **)(SUCC_BP(bp)))


//////////////////////////////////////////
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t asize);
static void insert_node(void *bp, size_t size);
static void delete_node(void *bp);

/////////////////////////////////////////
static char *heap_listp = 0;
static char *link_head[10];
/*
* mm_init - initialize the malloc package.
* The return value should be -1 if there was a problem in performing the initialization, 0 otherwise
*/
int mm_init(void)
{
	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
		return -1;
	}

	int i = 0;
	for (; i < MAXLIST; i++)
		link_head[i] = NULL;	

	PUT(heap_listp, 0);
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
	heap_listp += (2 * WSIZE);
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
		return -1;
	}
	return 0;
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
	if (size == 0) return NULL;

	if (size <= DSIZE) {
		asize = 2 * (DSIZE);
	}
	else {
		asize = (DSIZE)*((size + (DSIZE)+(DSIZE - 1)) / (DSIZE));
	}

	if ((bp = find_fit(asize)) == NULL) {
		extendsize = MAX(asize, CHUNKSIZE);
		if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
			return NULL;
		}
	}

	place(bp, asize);
	return bp;
}


/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *bp)
{
	if (bp == 0)
		return;

	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	insert_node(bp, size);

	coalesce(bp);
}


/*
* mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *bp, size_t size)
{
	if (bp == NULL)
	{
		void* bp = mm_malloc(size);
		return bp;
	}
	else if (size == 0)
	{
		mm_free(bp);
		return bp;
	}
	else
	{
		size_t old_size = GET_SIZE(HDRP(bp));

		if (old_size >= size)
			return bp;
		else
		{
			size_t csize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
			int isEmpty = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

			if (csize >= size - old_size && isEmpty == 0)
			{
				delete_node(bp);
				PUT(HDRP(bp), PACK((old_size + csize), 1));
				PUT(FTRP(bp), PACK((old_size + csize), 1));
				return bp;
			}
			else
			{
				void* new_block = mm_malloc(size);
				memcpy(new_block, bp, GET_SIZE(HDRP(bp)));
				mm_free(bp);
				return new_block;
			}
		}
	}

}

static int findlink(size_t size)
{
	if (size <= 8)
		return 0;
	else if (size <= 16)
		return 1;
	else if (size <= 32)
		return 2;
	else if (size <= 64)
		return 3;
	else if (size <= 128)
		return 4;
	else if (size <= 256)
		return 5;
	else if (size <= 512)
		return 6;
	else if (size <= 2048)
		return 7;
	else if (size <= 4096)
		return 8;
	else
		return 9;
}


static void insert_node(void *bp, size_t size)
{
	void* prev_bp = NULL;
	int listNum = findlink(size);
	void* search_bp = link_head[listNum];

	while (search_bp != NULL && (size > GET_SIZE(HDRP(search_bp))))
	{
		prev_bp = search_bp;
		search_bp = SUCC(search_bp);
	}
	
	if (search_bp == NULL)
	{
		if (prev_bp == NULL)
		{
			SET_BP(PRED_BP(bp), NULL);
			SET_BP(SUCC_BP(bp), NULL);
			link_head[listNum] = bp;
		}
		else
		{
			SET_BP(PRED_BP(bp), prev_bp);
			SET_BP(SUCC_BP(prev_bp), bp);
			SET_BP(SUCC_BP(bp), NULL);
		}
	}
	else
	{
		if (prev_bp == NULL)
		{
			SET_BP(PRED_BP(bp), NULL);
			SET_BP(SUCC_BP(bp), search_bp);
			SET_BP(PRED_BP(search_bp), bp);
		}
		else
		{
			SET_BP(PRED_BP(bp), prev_bp);
			SET_BP(SUCC_BP(bp), search_bp);
			SET_BP(PRED_BP(search_bp), bp);
			SET_BP(SUCC_BP(prev_bp), bp);
		}
	}
}


static void delete_node(void *bp)
{
	int size = GET_SIZE(HDRP(bp));
	int listNum = findlink(size);
	
	if (PRED(bp) == NULL)
	{
		if (SUCC(bp) == NULL)
			link_head[listNum] = NULL;
		else
		{
			SET_BP(PRED_BP(SUCC(bp)), NULL);
			link_head[listNum] = SUCC(bp);
		}

	}
	else
	{
		if (SUCC(bp) == NULL)
			SET_BP(SUCC_BP(PRED(bp)), NULL);
		else
		{
			SET_BP(SUCC_BP(PRED(bp)), SUCC(bp));
			SET_BP(PRED_BP(SUCC(bp)), PRED(bp));
		}
	}
}


static void *extend_heap(size_t words) {
	char *bp;
	size_t size;

	size = (words % 2) ? (words + 1)*WSIZE : words * WSIZE;
	if ((bp = mem_sbrk(size)) == (void *)-1)
		return NULL;

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

	insert_node(bp, size);

	return coalesce(bp);
}


static void *coalesce(void *bp) {
	size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) {
		return bp;
	}
	else if (prev_alloc && !next_alloc) {
		delete_node(bp);
		delete_node(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if (!prev_alloc && next_alloc) {
		delete_node(bp);
		delete_node(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	else {
		delete_node(bp);
		delete_node(PREV_BLKP(bp));
		delete_node(NEXT_BLKP(bp));
		size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	insert_node(bp, size);
	return bp;
}


static void place(void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp));
	size_t reminder = csize - asize;
	if (reminder >= (2 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		PUT(HDRP(bp), PACK(reminder, 0));
		PUT(FTRP(bp), PACK(reminder, 0));
		insert_node(NEXT_BLKP(bp), reminder);
	}
	else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}


static void *find_fit(size_t size)
{
	int listNum = findlink(size);
	void* bp = link_head[listNum];

	while (bp != NULL)
	{
		if (GET_SIZE(HDRP(bp)) > size)
			return bp;
		bp = SUCC(bp);
	}

	return NULL;
}
