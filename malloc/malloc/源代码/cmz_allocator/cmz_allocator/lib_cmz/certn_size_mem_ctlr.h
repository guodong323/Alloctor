//
//  certn_size_mem_ctlr.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/21.
//

#ifndef certn_size_mem_ctlr_h
#define certn_size_mem_ctlr_h

#include <stdio.h> 

extern int64_t certn_size_mem_ctlr_mmap_cnt;

typedef struct certn_size_mem {
    // 指向下一块可用的内存
    struct certn_size_mem *next;
} certn_size_mem;

typedef struct certn_size_mem_page {
    char mem[0];
    // next 和 last 使全部的 mem_page 构成一个链表
    struct certn_size_mem_page *next, *last;
    // 该 page 中未使用的块的链表
    certn_size_mem *unused_mem_head;
    // 该 page 总共占多少内存
    size_t page_size;
    // 该 page 已经分配过多少次内存了，当 mark 变为0时，可以考虑把该 page 归还操作系统
    size_t mark;
    // mem[brk] 指向的地址可以拿来分配
    size_t brk;
} certn_size_mem_page;

// mem_page 构成的链表的虚头尾节点
typedef struct certn_size_mem_page_vht {
    char mem[0];
    struct certn_size_mem_page *next, *last;
} certn_size_mem_page_vht;

typedef struct certn_size_mem_ctlr {
    // 满足 hot_page->brk <= hot_page->page_size，
    // 需要分配空间时，先去 mem_page 构成的链表获取内存，
    // 无法获取到内存时，去hot_page 获取。
    certn_size_mem_page *hot_page;
    // mem_page 构成的链表的虚头尾节点
    certn_size_mem_page_vht page_head, page_tail;
    // certn_size_mem_ctlr 分配出去的内存块的大小
    size_t mem_size;
    // 已经分配多少内存块出去了
    size_t used_mem_cnt;
    // 全部的 pages 总共占有了多少个 4KB 内存
    size_t total_page_cnt;
    // 锁，确保线程安全
    pthread_mutex_t lock;
} certn_size_mem_ctlr;

/// ctlr 初始化为 mem_size 大小内存的管理者
void certn_size_mem_ctlr_init(certn_size_mem_ctlr *ctlr, size_t mem_size);
/// 通过 ctlr 获取固定大小的内存
void* certn_size_mem_ctlr_alloc_mem(certn_size_mem_ctlr *ctlr);
/// 通过 ctlr 释放固定大小的内存
void certn_size_mem_ctlr_free_mem(certn_size_mem_ctlr *ctlr, void *mem);
/// 释放 ctlr 占据的全部 page 的内存给操作系统
void certn_size_mem_ctlr_free_all_mem(certn_size_mem_ctlr *ctlr);
/// 测试用
void print_certn_size_mem_ctlr(certn_size_mem_ctlr const * const ctlr);
#endif /* certn_size_mem_ctlr_h */

/*
 固定内存分配: 16 32 48 64 80 96 112 128
 
 */
