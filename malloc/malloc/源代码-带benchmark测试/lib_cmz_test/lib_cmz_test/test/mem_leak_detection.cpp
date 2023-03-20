//
//  mem_leak_detection.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/6/4.
//

#include "mem_leak_detection.h"
#include <sys/mman.h>
#include <pthread.h>
#include <map>

#define getMem(size) mmap(NULL, (size), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0)

#define rmMem(addr, size) munmap((addr), (size))

static pthread_mutex_t lock = PTHREAD_ONCE_INIT;
// 标记 mmap 和 munmap 的次数
static int64_t mem_mark = 0;
static std::map<void*, size_t> mem_map;

void *mld_getMem(size_t size) {
    pthread_mutex_lock(&lock);
    void*p = getMem(size);
    // 记录每一个地址对应的 mmap 获取的内存块大小
    mem_map[p] = size;
    ++mem_mark;
    pthread_mutex_unlock(&lock);
    return p;
}

int mld_rmMem(void *addr, size_t size) {
    pthread_mutex_lock(&lock);
    // 满足 “mem_map.count(addr)!=0 && mem_map[addr]==size” 才是正确的
    assert(mem_map.count(addr)!=0 && mem_map[addr]==size);
    mem_map.erase(addr);
    --mem_mark;
    int ret = rmMem(addr, size);
    pthread_mutex_unlock(&lock);
    return ret;
}

int is_leak(void) {
    return mem_mark!=0;
}
