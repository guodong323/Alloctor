//
//  lib_cmz.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/13.
//

#include "lib_cmz.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/types.h>
#include "mem_ctlr.h"
#include "GlobalHeader.h"

#define malloc cmz_malloc
#define calloc cmz_calloc
#define free cmz_free
#define realloc cmz_realloc

#define MAX_MEM_SIZE (1lu<<47)

static size_t ctlr_init_token = 0;
static mem_ctlr ctlr;

static void init(void) {
    mem_ctlr_init(&ctlr);
}
// 分配 size 字节的空间
void* malloc(size_t size) {
    once(&ctlr_init_token, init);
    // 对齐
    size = alignN(size, MIN_MEM_SIZE);
    
    // blk 从 mem_start 开始分配内存给用户，所以最少分配 (SIZE_FREE - SIZE_USED) 的内存
    if (size < MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED) {
        size = MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED;
    }
    if (size >= MAX_MEM_SIZE) { return NULL; }
    mem_ctl_blk *blk = mem_ctlr_get_blk(&ctlr, size);
    // 如果为空，那么没有空间了
    if (blk==NULL) { return NULL; }
    assert(blk->addr_next);
    if (mem_ctl_blk_get_mem_size(blk) >= size + MEM_CTL_INFO_SIZE_FREE) {
        // ret_blk 是多出来的块，由于 mem_ctl_blk 的大小有对 MIN_MEM_SIZE 对齐，
        // 所以 ret_blk 的可用空间也是 MIN_MEM_SIZE 的整数倍。
        mem_ctl_blk *ret_blk = (mem_ctl_blk*)(blk->mem_start + size);
        // 更新两个的内存块的链接关系
        ret_blk->addr_next = blk->addr_next;
        ret_blk->addr_last = blk;
        blk->addr_next = ret_blk;
        
        // 处理 flags 和 blk是尾部节点的情况
        mem_ctl_blk_flags_init(&ret_blk->flags);
        if (blk->flags.is_addr_tail) {
            ret_blk->flags.is_addr_tail = 1;
            blk->flags.is_addr_tail = 0;
        }else {
            ret_blk->addr_next->addr_last = ret_blk;
        }
        ret_blk->sibling_last = ret_blk->sibling_next = NULL;
        ret_blk->flags.is_free = 1;
        // 将 ret_blk放回红黑树
        mem_ctlr_store_blk(&ctlr, ret_blk);
    }
    return blk->mem_start;
}

// 分配 count 个大小为 size 字节的空间，并把每一位初始化为 0
void* calloc(size_t count, size_t size) {
    once(&ctlr_init_token, init);
    void *ret = malloc(count * size);
    if (ret==NULL) { return NULL; }
    memset(ret, 0, count * size);
    return ret;
}

// 将 ptr 指向的空间扩展到 size 字节
void* realloc(void *ptr, size_t size) {
    if (ptr==NULL) { return calloc(1, size); }
    // 通过地址偏移，获得控制单元
    mem_ctl_blk *blk = mem_ctl_blk_from_mem_start(ptr);
    // 原来分配出去的空间大小
    const size_t old_size = mem_ctl_blk_get_mem_size(blk);
    // 如果新容量比已分配的更小，那么 return，不作进一步处理
    if (old_size >= size) { return ptr; }
    size = alignN(size, MIN_MEM_SIZE);
    
    // blk 从 mem_start 开始分配内存给用户，所以最少分配 (SIZE_FREE - SIZE_USED) 的内存
    if (size < MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED) {
        size = MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED;
    }
    
    if (size >= MAX_MEM_SIZE) { return NULL; }
    
    // 需要增加的容量
    size_t ext_size = size - old_size;
    // next_blk 是 blk 地址连续的下一个 blk
    mem_ctl_blk *next_blk = blk->addr_next;
    if (!mem_ctl_blk_is_addr_tail(blk) && mem_ctl_blk_is_free(next_blk)) {
        //  next_blk 的可用空间大于 ext_size，那么从可用空间截取空间，用于扩容
        if (mem_ctl_blk_get_total_size(next_blk) >= ext_size + MEM_CTL_INFO_SIZE_FREE) {
            // 内部完成链表的断开工作，从红黑树移除
            mem_ctlr_remove_blk_from_sibling_link(&ctlr, next_blk);
            // 分割
            mem_ctl_blk *new_next_blk = (mem_ctl_blk*)(((char*)next_blk) + ext_size);
            
            mem_ctl_blk_flags next_blk_flags = next_blk->flags;
            mem_ctl_blk *next_blk_addr_next = next_blk->addr_next;
            
            // 处理 new_next_blk 的 addr 链接
            new_next_blk->addr_last = blk;
            new_next_blk->addr_next = next_blk_addr_next;
            new_next_blk->sibling_next = new_next_blk->sibling_last = NULL;
            
            // 处理 new_next_blk 的 flags
            new_next_blk->flags = next_blk_flags;
            new_next_blk->flags.is_free = 1;
            if (next_blk_flags.is_addr_tail) {
                new_next_blk->flags.is_addr_tail = 1;
            }else {
                next_blk_addr_next->addr_last = new_next_blk;
            }
            blk->addr_next = new_next_blk;
            
            // 将blk->addr_next 放回红黑树
            assert(mem_ctl_blk_get_total_size(blk) >= MEM_CTL_INFO_SIZE_FREE);
            mem_ctlr_store_blk(&ctlr, new_next_blk);
            // next_blk 的可用空间不够，但是 next_blk 占据的总空间大于等于 ext_size
            // 那么把 next_blk 和 blk 合并
        }else if (mem_ctl_blk_get_total_size(next_blk) >= ext_size) {
            mem_ctlr_remove_blk_from_sibling_link(&ctlr, next_blk);
            blk->addr_next = next_blk->addr_next;
            if (next_blk->flags.is_addr_tail) {
                blk->flags.is_addr_tail = 1;
                next_blk->flags.is_addr_tail = 0;
            }else {
                next_blk->addr_next->addr_last = blk;
            }
        }else {
            // 如果上面的两个情况都不满足，那么跳转到 malloc_new_space 重新 malloc
            goto malloc_new_space;
        }
        return ptr;
    }
malloc_new_space:;
    void *new_space = malloc(size);
    memmove(new_space, ptr, old_size);
    free(ptr);
    return new_space;
}

// 回收 ptr 指向的空间
void free(void *ptr) {
    if (ptr) {
        mem_ctl_blk *blk = mem_ctl_blk_from_mem_start(ptr);
        mem_ctlr_store_blk(&ctlr, blk);
    }
}


