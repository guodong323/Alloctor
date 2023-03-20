//
//  mem_ctl_link.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#ifndef mem_ctl_link_h
#define mem_ctl_link_h
#include <stdio.h>
#include "mem_ctl_blk.h"



// 组织一个链表，链表内的所有内存块的空间大小都一样
typedef struct mem_ctl_link {
    // head->sibling_next == tail 代表 mem_ctl_link 为空，此时需要从红黑树移除这个 link
    mem_ctl_blk head, tail;
    // block 的大小 存在 rb_node_t
} mem_ctl_link;

// 对外提供的函数支持:
// 注意，移除 blk 的操作，都不会去 [判断移除后 link 是否为空，然后空link从红黑树移除]

void mem_ctl_link_init(mem_ctl_link* lk);
// 判断 link 是否为空
char mem_ctl_link_is_empty(mem_ctl_link *link);

// 从 link 中获取一个内存块，并把内存块从 link 里面移除
mem_ctl_blk* mem_ctl_link_pop(mem_ctl_link *link);

// 把 blk 插入 link
void mem_ctl_link_push(mem_ctl_link *link, mem_ctl_blk *blk);

// 把 blk 从其所归属的 mem_ctl_link 中断开
void mem_ctl_link_remove_blk(mem_ctl_blk *blk);

#endif /* mem_ctl_link_h */
