//
//  mem_ctlr.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#include "mem_ctlr.h"
#include "GlobalHeader.h"


long double tm = 0, ts = 0;

static void mem_ctlr_remove_blk_from_sibling_link_unlock(mem_ctlr *ctlr, mem_ctl_blk *blk) {
    if (mem_ctl_blk_is_last_one(blk)) {
        mem_ctl_link_remove_blk(blk);
        // 把 link 从红黑树移除
        ctlr->rb_root = rb_erase(ctlr->rb_root, mem_ctl_blk_get_mem_size(blk));
    }else {
        mem_ctl_link_remove_blk(blk);
    }
    ctlr->used_mem += mem_ctl_blk_get_total_size(blk);
}

static void mem_ctlr_remove_all_blk(mem_ctlr *ctlr) {
    rb_node *root = ctlr->rb_root;
    while (root) {
        mem_ctl_blk *tail = &root->data.tail;
        mem_ctl_blk *first = root->data.head.sibling_next;
        while (first!=tail) {
            mem_ctl_blk *temp = first;
            first = first->sibling_next;
            mem_ctl_blk_free(temp);
        }
        root = rb_erase_node(root, root);
    }
    ctlr->rb_root = root;
    ctlr->total_mem = 0;
}

/// 初始化
void mem_ctlr_init(mem_ctlr *ctlr) {
    ctlr->rb_root = NULL;
    ctlr->used_mem = ctlr->total_mem = 0;
    ctlr->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
}

// 从红黑树获取[内存大小] >= size 的 blk，只有系统内存不够时返回值为空
mem_ctl_blk* mem_ctlr_get_blk(mem_ctlr *ctlr, size_t size) {
    pthread_mutex_lock(&ctlr->lock);
#warning "size 大小检测"
    if (size < MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED) {
        size = MEM_CTL_INFO_SIZE_FREE - MEM_CTL_INFO_SIZE_USED;
    }
    rb_node *node = rb_search_most_close(ctlr->rb_root, size);
    mem_ctl_blk* blk;
    if (node==NULL) {
        size_t alc_size = alignN(ctlr->total_mem/2, PAGE_SIZE);
        // size:分配出去的大小
        // MEM_CTL_INFO_SIZE_USED 出来的 blk 的头部信息大小
        size_t min_alc_size = alignN(size + MEM_CTL_INFO_SIZE_USED, PAGE_SIZE);
        if (alc_size < min_alc_size) { alc_size = min_alc_size; }
        blk = mem_ctl_blk_new(alc_size);
        if (blk) {
            ctlr->total_mem += alc_size;
            ctlr->used_mem += alc_size;
        }
    }else {
        mem_ctl_link *lk = &node->data;
        blk = mem_ctl_link_pop(lk);
        if (mem_ctl_link_is_empty(lk)) {
            ctlr->rb_root = rb_erase_node(ctlr->rb_root, node);
        }
        ctlr->used_mem += mem_ctl_blk_get_total_size(blk);
    }
    assert(blk && (mem_ctl_blk_get_total_size(blk) >= MEM_CTL_INFO_SIZE_FREE));
    pthread_mutex_unlock(&ctlr->lock);
    return blk;
}


// 实现空闲块的合并与放回红黑树、在恰当时机将空闲块归还操作系统
void mem_ctlr_store_blk(mem_ctlr *ctlr, mem_ctl_blk* blk) {
    pthread_mutex_lock(&ctlr->lock);
    assert(blk && (mem_ctl_blk_get_total_size(blk) >= MEM_CTL_INFO_SIZE_FREE));
    
    mem_ctl_blk *nei_blk;
    // 检查上一块是否为空闲
    nei_blk = blk->addr_last;
    
    if (nei_blk && mem_ctl_blk_is_free(nei_blk)) {
        mem_ctlr_remove_blk_from_sibling_link_unlock(ctlr, nei_blk);
        mem_ctl_blk_merge_next(nei_blk);
        blk = nei_blk;
    }
    
    // 从 mem_ctl_blk_merge_next 的逻辑上看，先合并上一块，并不会导致 bug
    // 检查下一块是否为空闲
    nei_blk = blk->addr_next;
    if (!mem_ctl_blk_is_addr_tail(blk) && mem_ctl_blk_is_free(nei_blk)) {
        mem_ctlr_remove_blk_from_sibling_link_unlock(ctlr, nei_blk);
        mem_ctl_blk_merge_next(blk);
        
    }
    blk->flags.is_free = 1;
    ctlr->used_mem -= mem_ctl_blk_get_total_size(blk);
    assert(blk && (mem_ctl_blk_get_total_size(blk) >= MEM_CTL_INFO_SIZE_FREE));
    if (blk->flags.is_addr_head && blk->flags.is_addr_tail) {
        const size_t blk_size = mem_ctl_blk_get_total_size(blk);
        assert(blk_size%PAGE_SIZE==0);
        double rate = (ctlr->used_mem)/(ctlr->total_mem - blk_size + 1.0);
        if (rate < 0.77) {
            mem_ctl_blk_free(blk);
            ctlr->total_mem -= blk_size;
            if (ctlr->used_mem==0) {
                mem_ctlr_remove_all_blk(ctlr);
            }
            pthread_mutex_unlock(&ctlr->lock);
            return;
        }
    }
    
    mem_ctl_link *lk;
    size_t blk_mem_size = mem_ctl_blk_get_mem_size(blk);
    rb_node *node = rb_search(ctlr->rb_root, blk_mem_size);
    if (!node) {
        node = rb_node_new_node();
        node->key = blk_mem_size;
        ctlr->rb_root = rb_insert_node(ctlr->rb_root, node);
    }
    lk = &node->data;
    blk->flags.is_free = 1;
    mem_ctl_link_push(lk, blk);
    pthread_mutex_unlock(&ctlr->lock);
}

// 把 blk 从其所归属的 mem_ctl_link 中断开
void mem_ctlr_remove_blk_from_sibling_link(mem_ctlr *ctlr, mem_ctl_blk *blk) {
    pthread_mutex_lock(&ctlr->lock);
    mem_ctlr_remove_blk_from_sibling_link_unlock(ctlr, blk);
    pthread_mutex_unlock(&ctlr->lock);
}
