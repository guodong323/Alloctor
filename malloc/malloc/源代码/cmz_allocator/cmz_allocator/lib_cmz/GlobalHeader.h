//
//  GlobalHeader.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/13.
//

#ifndef GlobalHeader_h
#define GlobalHeader_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/errno.h>


// 最小内存块大小，要求是2的k次方
#define MIN_MEM_SIZE (1lu << 3)
// 一页的大小
#define PAGE_SIZE (1lu<<12)

#define dbugLog(fmt, ...) printf("[%d]: " fmt "\n" , __LINE__, ##__VA_ARGS__)

#define CMZMAX(a, b) ((a) > (b) ? (a) : (b))
// if 更倾向于 1
#define T1(x) (__builtin_expect(!!(x), 1))
// if 更倾向于 0
#define T0(x) (__builtin_expect(!!(x), 0))

#define getMem(size) mmap(NULL, (size), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0)

#define rmMem(addr, size) munmap((addr), (size))


/// 使整个程序运行过程 once 函数中 f 只调用一次
/// @param token 必须为静态变量，初始化为 0 后外界不能修改它
void once(size_t *token, void(*f)(void));

/// 将 size 对齐到 n 的倍数，要求 n 为 2的n次方
size_t alignN(size_t size, size_t n);
extern int64_t mmap_cnt;


#ifdef __cplusplus
}
#endif
#endif /* GlobalHeader_h */
