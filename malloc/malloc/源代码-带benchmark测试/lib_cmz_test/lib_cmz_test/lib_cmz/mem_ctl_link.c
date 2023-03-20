//
//  mem_ctl_link.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#include "mem_ctl_link.h"
#include "GlobalHeader.h"

void mem_ctl_link_init(mem_ctl_link* lk) {
    assert(lk);
    mem_ctl_blk_flags_init(&lk->head.flags);
    mem_ctl_blk_flags_init(&lk->tail.flags);
    
    lk->head.flags.is_sibling_head = lk->tail.flags.is_sibling_tail = 1;
    lk->head.flags.is_free = lk->tail.flags.is_free = 0;
    // 处理 head 和 tail 的连接关系
    lk->head.sibling_next = &lk->tail;
    lk->tail.sibling_last = &lk->head;
    
    lk->head.sibling_last = lk->tail.sibling_next
    = lk->head.addr_next = lk->head.addr_last
    = lk->tail.addr_next = lk->tail.addr_last = NULL;
}
// 判断 link 是否为空
char mem_ctl_link_is_empty(mem_ctl_link *link) {
    assert(link);
    return link->head.sibling_next == &link->tail;
}

// 从 link 中获取一个内存块，并把内存块从 link 里面移除
mem_ctl_blk* mem_ctl_link_pop(mem_ctl_link *link) {
    assert(link && !mem_ctl_link_is_empty(link));
    mem_ctl_blk *ret = link->head.sibling_next;
    mem_ctl_link_remove_blk(ret);
    return ret;
}

// 把 blk 插入 link
void mem_ctl_link_push(mem_ctl_link *link, mem_ctl_blk *blk) {
    assert(link && blk);
    mem_ctl_blk *head_next = link->head.sibling_next;
    blk->sibling_last = &link->head;
    blk->sibling_next = head_next;
    
    link->head.sibling_next = blk;
    head_next->sibling_last = blk;
    blk->flags.is_free = 1;
}

void mem_ctl_link_remove_blk(mem_ctl_blk *blk) {
    assert(blk && mem_ctl_blk_is_free(blk));
    mem_ctl_blk *last = blk->sibling_last, *next = blk->sibling_next;
    // 理论上，会来调用这个函数的 blk 的 last 和 next，应该都不为空，安全起见，还是加个 if
    last->sibling_next = next;
    next->sibling_last = last;
    blk->sibling_last = blk->sibling_next = NULL;
    blk->flags.is_free = 0;
}
