#include "memlib.h" // memlib.c包提供的存储器系统模型，该模型允许我们在不干涉已存在的malloc包的情况下运行分配器
#include "mm_latest.h"
#include <assert.h>
#include <string.h>

#define DEBUG

/* 常量定义 */

const unsigned int WORD = 4; // 字长，同时也是头部和脚部的大小
const unsigned int DOUBLE_WORD = 8;
const unsigned int DEFAULTSIZE = 1 << 12;
#define MAXLIST 10

/* 类型替换 */

typedef char* address_t; // 地址的类型
typedef unsigned int offset_t; // 地址偏移的类型

/* 静态内联函数（替换宏定义）*/

// 返回两者中较大的一个
static inline size_type max(size_type x, size_type y)
{
    return x > y ? x : y;
}

// 返回头部内容，头部最低位表示空闲块还是已分配的块
static inline size_type pack(size_type x, bool alloc)
{
    return x | alloc;
}

// 在地址address上读、写一个四个字节大小的内容
static inline size_type get(address_t p)
{
    return *((size_type *)p);
}

static inline void put(address_t p, size_type val)
{
    *(size_type *)p = val;
}

// 返回头部或脚部表示的块大小
static inline size_type get_size(address_t p)
{
    size_type x = get(p);
    size_type mask = ~0x7; // 遮盖最后三位
    return x & mask;
}

// 是否是已分配的块
static inline bool is_alloc(address_t p)
{
    size_type x = get(p);
    size_type mask = 0x1; // 遮盖除了最低位外的所有位
    return x & mask;
}

// 返回块的头部地址, 这里的参数指向的是头部后的正文开头
static inline address_t head(address_t p)
{
    return p - WORD;
}

// 返回块的脚部地址
static inline address_t foot(address_t p)
{
    offset_t t = get_size(head(p)) - DOUBLE_WORD;
    return p + t;
}

// 返回下一个块的正文地址
static inline address_t next_block(address_t p)
{
    offset_t t = get_size(head(p));
    return p + t;
}

// 返回上一个块的正文地址
static inline address_t prev_block(address_t p)
{
    offset_t t = get_size(p - DOUBLE_WORD);
    return p - t;
}

// 用于内存对齐
static inline size_type round_up(size_type size)
{
    // DOUBLE_WORD - 1是为了大于舍入整数一点
    return DOUBLE_WORD * ((size + DOUBLE_WORD + (DOUBLE_WORD - 1)) / DOUBLE_WORD);
}

static inline void set_node(address_t bp, address_t ptr)
{
    *((address_t*)bp) = ptr;
}

static inline address_t prev_ptr(address_t ptr)
{
    return ptr;
}

static inline address_t next_ptr(address_t ptr)
{
    return ptr + DOUBLE_WORD;
}

static inline address_t next_node(address_t ptr)
{
    return *((address_t*)ptr);
}

static inline address_t prev_node(address_t ptr)
{
    return *((address_t*)(ptr + DOUBLE_WORD));
}

/* 静态全局变量 */
static address_t heap_listp = 0; // 序言块，static对外隐藏变量
static address_t link_head[MAXLIST] = {0}; // C中这里的MAXLIST只能用define常量而不能用const变量替代，而C++中可以

/* 声明辅助函数 */
static void* extend_heap(size_type size);
static void* merge(address_t ptr);
static void* find_fit(size_type size);
static void place(address_t ptr, size_type actual_size);
static size_type find_link(size_type size);
static void insert_node(address_t ptr, size_type size);
static void delete_node(address_t ptr);

/* 接口函数 */

// 初始化序言块

bool mm_init()
{
    mem_init();
    if ((heap_listp = mem_sbrk(4 * WORD)) == (void*)(-1))
        return false;

    put(heap_listp, 0); // 这里是为了第一个块的正文部分地址为8的倍数，符合内存对齐要求
    put(heap_listp + WORD, pack(DOUBLE_WORD, true)); // 序言块头
    put(heap_listp + 2* WORD, pack(DOUBLE_WORD, true)); // 序言块脚
    put(heap_listp + 3 * WORD, pack(0, true)); // 结尾块

    heap_listp += DOUBLE_WORD;
    if (extend_heap(DEFAULTSIZE) == NULL)
        return false;
    else
        return true;
}

void* mm_malloc(size_type size)
{
    if (size == 0)
        return NULL;

    size_type actual_size;

    if (size < DOUBLE_WORD) // 最少分配16字节
        actual_size = 2 * DOUBLE_WORD;
    else
        actual_size = round_up(size);

    address_t ptr;
    if ((ptr = find_fit(actual_size)) != NULL)
    {
        place(ptr, actual_size);
        return (void*)ptr;
    }

    size_type extendsize = max(actual_size, DEFAULTSIZE);
    if ((ptr = extend_heap(extendsize)) == NULL)
        return NULL;

    place(ptr, actual_size);
    return (void*)ptr;
}

void* mm_realloc(void* ptr, size_type size)
{
    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    address_t bp = (address_t)ptr;
    size_type old_size = get_size(head(bp));

    if (old_size >= size)
        return (void*)bp;

    size_type next_size = get_size(head(next_block(bp)));
    bool alloc = is_alloc(head(next_block(bp)));
    if (!alloc && next_size >= size - old_size)
    {
        delete_node(bp);
        put(head(bp), pack(old_size + next_size, true));
        put(foot(bp), pack(old_size + next_size, true));
        return (void*)bp;
    }
    else
    {
        void* new_block = mm_malloc(size);
        memcpy(new_block, (void*)bp, get_size(head(bp)));
        mm_free((void*)bp);
        return new_block;
    }

}

void mm_free(void* ptr)
{
    if (ptr == NULL)
        return;

    address_t bp = (address_t)ptr;
    size_type s = get_size(head(bp));
    put(head(bp), pack(s, false));
    put(foot(bp), pack(s, false));
    insert_node(bp, s);

    merge(bp);
}


/* 辅助函数 */

static void* extend_heap(size_type size)
{
    address_t bp = mem_sbrk(size);
    if (bp == (void*)-1)
        return NULL;

    put(head(bp), pack(size, false));
    put(foot(bp), pack(size, false));
    put(head(next_block(bp)), pack(0,true));

    insert_node(bp, size);

    return merge(bp);
}

static void* merge(address_t ptr)
{
    bool is_prev_alloc = is_alloc(head(prev_block(ptr)));
    bool is_next_alloc = is_alloc(head(next_block(ptr)));
    size_type size = get_size(head(ptr));

    if (is_prev_alloc && is_next_alloc)
        return ptr;
    else if (is_prev_alloc && !is_next_alloc)
    {
        delete_node(ptr);
        delete_node(next_block(ptr));
        size += get_size(head(next_block(ptr)));
        put(head(ptr), pack(size, false));
        put(foot(ptr), pack(size, false));
    }
    else if (!is_prev_alloc && is_next_alloc)
    {
        delete_node(ptr);
        delete_node(prev_block(ptr));
        size += get_size(head(prev_block(ptr)));
        put(foot(ptr), pack(size, false));
        put(head(prev_block(ptr)), pack(size, false));
        ptr = prev_block(ptr);
    }
    else
    {
        delete_node(ptr);
        delete_node(prev_block(ptr));
        delete_node(next_block(ptr));
        size += get_size(head(prev_block(ptr))) + get_size(head(next_block(ptr)));
        put(foot(next_block(ptr)), pack(size, false));
        put(head(prev_block(ptr)), pack(size, false));
        ptr = prev_block(ptr);
    }

    insert_node(ptr, size);
    return ptr;
}

static void* find_fit(size_type size)
{
    size_type list_num = find_link(size);
    address_t bp = link_head[list_num];

    while (bp != NULL)
    {
        if (get_size(head(bp)) >= size)
            return bp;
        else
            bp = next_node(bp);
    }

    return NULL;
}

static void place(address_t ptr, size_type actual_size)
{
    size_type origin = get_size(head(ptr));
    size_type remain = origin - actual_size;

    if (remain >= 2 * DOUBLE_WORD)
    {
        put(head(ptr), pack(actual_size, true));
        put(foot(ptr), pack(actual_size, true));
        ptr = next_block(ptr);
        put(head(ptr), pack(remain, false));
        put(foot(ptr), pack(remain, false));
    }
    else
    {
        put(head(ptr), pack(origin, true));
        put(foot(ptr), pack(origin, true));
    }
}

static size_type find_link(size_type size)
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

static void insert_node(address_t ptr, size_type size)
{
    size_type list_num = find_link(size);
    address_t head_node = link_head[list_num];
    address_t last_node = NULL;

    while (head_node != NULL && (size > get_size(head(head_node))))
    {
        last_node = head_node;
        head_node = next_node(head_node);
    }

    if (head_node == NULL)
    {
        if (last_node == NULL)
        {
            set_node(prev_ptr(ptr), NULL);
            set_node(next_ptr(ptr), NULL);
            link_head[list_num] = ptr;
        }
        else
        {
            set_node(prev_ptr(ptr), last_node);
            set_node(next_ptr(last_node), ptr);
            set_node(next_ptr(ptr), NULL);
        }
    }
    else
    {
        if (last_node == NULL)
        {
            set_node(prev_ptr(ptr), NULL);
            set_node(next_ptr(ptr), head_node);
            set_node(prev_ptr(head_node), ptr);
        }
        else
        {
            set_node(prev_ptr(ptr), last_node);
            set_node(next_ptr(ptr), head_node);
            set_node(prev_ptr(head_node), ptr);
            set_node(next_ptr(last_node), ptr);
        }
    }


}

static void delete_node(address_t ptr)
{
    size_type size = get_size(head(ptr));
    int list_num = find_link(size);

    if (prev_node(ptr) == NULL)
    {
        if (next_node(ptr) == NULL)
            link_head[list_num] = NULL;
        else
        {
            set_node(prev_ptr(next_node(ptr)), NULL);
            link_head[list_num] = next_node(ptr);
        }
    }
    else
    {
        if (next_node(ptr) == NULL)
            set_node(next_ptr(prev_node(ptr)), NULL);
        else
        {
            set_node(next_ptr(prev_node(ptr)), next_node(ptr));
            set_node(prev_ptr(next_node(ptr)), prev_node(ptr));
        }
    }
}


#ifdef DEBUG

int main(int argc, char* argv[])
{

    return 0;
}

#endif