//
//  mem_ctl_blk.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#include "mem_ctl_blk.h"
#include "GlobalHeader.h"


void mem_ctl_blk_flags_init(mem_ctl_blk_flags *flags) {
    assert(flags);
    flags->bits = 0;
}

/// 获取可分配给用户使用的空间大小
size_t mem_ctl_blk_get_mem_size(mem_ctl_blk *blk) {
    assert(blk);
    return ((char*)blk->addr_next) - ((char*)blk->mem_start);
}

// 获取整个 blk 的大小
size_t mem_ctl_blk_get_total_size(mem_ctl_blk *blk) {
    assert(blk);
    return ((char*)blk->addr_next) - (char*)blk;
}

// 通过地址偏移由 mem_ctl_blk->mem_start 获取mem_ctl_blk
mem_ctl_blk* mem_ctl_blk_from_mem_start(char *data) {
    assert(data);
    return (mem_ctl_blk*)(data - MEM_CTL_INFO_SIZE_USED);
}
// 判断 blk 是否是空闲的
int mem_ctl_blk_is_free(mem_ctl_blk *blk) {
    assert(blk);
    return blk->flags.is_free;
}

// 将两个空闲的 blk 合并，返回值为新 blk，调用前提是两个 blk
// 都空闲(不在sibling链表中)，且blk1->addr_next = blk2
void mem_ctl_blk_merge_next(mem_ctl_blk *blk) {
    mem_ctl_blk *next = blk->addr_next;
    assert(!mem_ctl_blk_is_addr_tail(blk));
    mem_ctl_blk *new_next = next->addr_next;
    blk->addr_next = new_next;
    if (next->flags.is_addr_tail) {
        blk->flags.is_addr_tail = 1;
    }else {
        new_next->addr_last = blk;
    }
    
}
/// 获取一个内存控制块，该 API 通过系统调用实现，如果没有内存那么返回 NULL
mem_ctl_blk * mem_ctl_blk_new(size_t alc_size) {
    assert(alc_size%4096==0 && alc_size>=4096);
    char *mem = getMem(alc_size);
    if (mem==MAP_FAILED) {
        return NULL;
    }
    mem_ctl_blk* blk = (mem_ctl_blk*)mem;
    mem_ctl_blk_flags_init(&blk->flags);
    blk->addr_last = NULL;
    blk->addr_next = (mem_ctl_blk*)(mem + alc_size);
    blk->sibling_last = blk->sibling_next = NULL;
    blk->flags.is_addr_head = blk->flags.is_addr_tail = 1;
    blk->flags.is_free = 0;
    return blk;
}

void mem_ctl_blk_free(mem_ctl_blk *blk) {
    assert(blk->flags.is_addr_head && blk->flags.is_addr_tail);
    rmMem(blk, mem_ctl_blk_get_total_size(blk));
}


// 判断 blk 是 sibling 的否是虚头部
int mem_ctl_blk_is_sibling_head(mem_ctl_blk*blk) {
    assert(blk);
    return blk->flags.is_sibling_head;
}
/*
 mem = 0x100223000, 4096
 mem = 0x100224000, 4096
 mem = 0x100225000, 4096
 */
// 判断 blk 是 sibling 的否是虚尾部
int mem_ctl_blk_is_sibling_tail(mem_ctl_blk*blk) {
    assert(blk);
    return blk->flags.is_sibling_tail;
}

int mem_ctl_blk_is_last_one(mem_ctl_blk*blk) {
    assert(blk && mem_ctl_blk_is_free(blk));
    return mem_ctl_blk_is_sibling_head(blk->sibling_last) && mem_ctl_blk_is_sibling_tail(blk->sibling_next);
}

// 判断 blk 是 addr 的否是虚头部
int mem_ctl_blk_is_addr_head(mem_ctl_blk*blk) {
    assert(blk);
    return blk->flags.is_addr_head;
}

// 判断 blk 是 addr 的否是虚尾部
int mem_ctl_blk_is_addr_tail(mem_ctl_blk*blk) {
    assert(blk);
    return blk->flags.is_addr_tail;
}

/*
 #define IS_FREE_MASK (1lu<<1)

 #define IS_HEAD_MASK (1lu<<2)

 #define IS_TAIL_MASK (1lu<<3)

 #define getBit(val, idx) (((val)>>(idx))&1lu)

 #define setBit1(val, idx) ((val) |= (1lu << (idx)))

 #define setBit0(val, idx) ((val) &= !(1lu << (idx)))
 */
