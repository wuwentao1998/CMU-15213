// 为了便于测试，假设本函数用于32位操作系统

/* 类型替换 */
typedef enum {false = 0, true = 1} bool;
typedef unsigned int size_type; // 4个字节大小


/* 接口函数 */
bool mm_init();

void* mm_malloc(size_type size);

void* mm_realloc(void* ptr, size_type size);

void mm_free(void* ptr);