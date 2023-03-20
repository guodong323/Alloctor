//
//  mem_ctl_blk.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#ifndef mem_ctl_blk_h
#define mem_ctl_blk_h

#include <stdio.h>



#define ADDR_BIT_CNT (sizeof(size_t)*8 - 3)

// 处于被用户使用状态的 blk 的控制字段的大小
#define MEM_CTL_INFO_SIZE_USED __builtin_offsetof(mem_ctl_blk, mem_start)

// 处于未使用使用状态的 blk 的控制字段的大小
#define MEM_CTL_INFO_SIZE_FREE sizeof(mem_ctl_blk)

typedef union mem_ctl_blk_flags {
    size_t bits;
    struct {
        // 标记 blk 是否正空闲，这个标记位的修改规则是：谁直接改变 sibling 链，谁修改
        size_t is_free : 1;
        // 标记 blk 是否是 sibling_head
        size_t is_sibling_head : 1;
        // 标记 blk 是否是 sibling_tail
        size_t is_sibling_tail : 1;
        // 谁改变 addr 链，谁修改
        size_t is_addr_head : 1;
        size_t is_addr_tail : 1;
//            size_t reserved : 61;
    };
} mem_ctl_blk_flags;

typedef struct mem_ctl_blk {
    // 标记位
    mem_ctl_blk_flags flags;
    // addr_last、addr_next 指向内存地址相连续的上一块、下一块、free 时要用，使用虚头尾机制
    struct mem_ctl_blk *addr_last, *addr_next;
    // align 用来占位，使 mem_ctl_blk 占用的空间对 MIN_MEM_SIZE 对齐
    size_t align[0];
    // 从 data 指向的空间起，都是拿来分配出去的
    char mem_start[0];
    // 指向[块大小]一样的上一块，下一块，在红黑树中的节点才会用到
    struct mem_ctl_blk *sibling_last, *sibling_next;
} mem_ctl_blk;


// 对外提供的函数支持:

// 获取可分配给用户使用的空间大小
size_t mem_ctl_blk_get_mem_size(mem_ctl_blk *blk);

// 获取整个 blk 的大小
size_t mem_ctl_blk_get_total_size(mem_ctl_blk *blk);

// 通过地址偏移由 mem_ctl_blk->mem_start 获取mem_ctl_blk
mem_ctl_blk* mem_ctl_blk_from_mem_start(char *data);


// MARK: - 合并与拆分
// 将两个空闲的 blk 合并，返回值为新 blk，
//调用前提是 blk 与 blk->addr_next 都空闲(不在sibling链表中)
void mem_ctl_blk_merge_next(mem_ctl_blk *blk);


//MARK: - 创建、回收、初始化
// 将内存块 初始化为刚 alloc 的状态，从另一个 blk 分割出来的 blk，不能调用这个函数。
mem_ctl_blk* mem_ctl_blk_new(size_t alc_size);

void mem_ctl_blk_free(mem_ctl_blk *blk);

void mem_ctl_blk_flags_init(mem_ctl_blk_flags *flags);
//MARK: - 状态判定

// 判断 blk 是否是空闲的
int mem_ctl_blk_is_free(mem_ctl_blk *blk);

// 判断 blk 是 sibling 的否是虚头部
//int mem_ctl_blk_is_sibling_head(mem_ctl_blk*blk);
//
//// 判断 blk 是 sibling 的否是虚尾部
//int mem_ctl_blk_is_sibling_tail(mem_ctl_blk*blk);

/// 判断 blk 是否是 sibling 中的最后一块
int mem_ctl_blk_is_last_one(mem_ctl_blk*blk);

// 判断 blk 是 addr 的否是虚头部
int mem_ctl_blk_is_addr_head(mem_ctl_blk*blk);

// 判断 blk 是 addr 的否是虚尾部
int mem_ctl_blk_is_addr_tail(mem_ctl_blk*blk);

#endif /* mem_ctl_blk_h */
