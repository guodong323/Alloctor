//
//  mem_ctlr.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#ifndef mem_ctlr_h
#define mem_ctlr_h
#include <stdio.h>
#include <pthread.h>
#include "rb_tree_support.h"

extern long double tm, ts;

typedef struct mem_ctlr {
    rb_node *rb_root;
    // 已分配出去的空间
    size_t used_mem;
    // 总空间
    size_t total_mem;
    pthread_mutex_t lock;
    int a;
} mem_ctlr;

// 提供接口：
/// 初始化
void mem_ctlr_init(mem_ctlr *ctlr);
// 从红黑树获取[内存大小] >= size 的 blk，只有系统内存不够时返回值为空
mem_ctl_blk* mem_ctlr_get_blk(mem_ctlr *ctlr, size_t size);
// 实现空闲块的合并与放回红黑树、在恰当时机将空闲块归还操作系统
void mem_ctlr_store_blk(mem_ctlr *ctlr, mem_ctl_blk* blk);

// 把 blk 从其所归属的 mem_ctl_link 中断开
void mem_ctlr_remove_blk_from_sibling_link(mem_ctlr *ctlr, mem_ctl_blk *blk);
#endif /* mem_ctlr_h */
