//
//  certn_size_mem_ctlr.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/21.
//

#include "certn_size_mem_ctlr.h"
#include "GlobalHeader.h"
#include <sys/mman.h>
#include <pthread/pthread.h>
#include <math.h>


//MARK: - 内部使用的函数
/// 获取一个占据 page_cnt*PAGE_SIZE B内存的 page
static certn_size_mem_page* mem_page_new(size_t page_cnt) {
    const size_t mem_size = page_cnt*PAGE_SIZE;
    certn_size_mem_page *page = getMem(mem_size);
    if (T0(page==MAP_FAILED)) {
        return NULL;
    }
    page->last = page->next = NULL;
    page->unused_mem_head = NULL;
    page->page_size = mem_size;
    page->mark = 0;
    page->brk = alignN(sizeof(certn_size_mem_page), MIN_MEM_SIZE);
    return page;
}

/// 搜索 mem 所属于的 certn_size_mem_page
static certn_size_mem_page* mem_ctlr_get_page_of_mem(certn_size_mem_ctlr *ctlr, certn_size_mem *mem) {
    certn_size_mem_page *page = ctlr->page_head.next;
    certn_size_mem_page *tail = (certn_size_mem_page*)(&ctlr->page_tail);
    char * const wrap_mem = (char*)mem;
    // 去 mem_page 链表中线性搜索，性能不会差，因为从内存分配的设计来看，
    // 如果 mem_ctlr 总共占据 2^60 B 的内存时，链表总共也就 100 个节点左右
    // 2^31 B 时有 50 个节点左右
    // 尝试过将线性搜索优化为二分搜索，运行时并没有大改变
    while (page!=tail) {
        char * const wrap_page = (char*)page;
        if (wrap_page <= wrap_mem && wrap_mem < (wrap_page+page->page_size)) {
            break;
        }
        page = page->next;
    }
    return page;
}

// 从 mem_link 获取块，当 mem_link 没有可用的块时，返回 NULL
static certn_size_mem* mem_ctlr_get_mem_form_mem_link(certn_size_mem_ctlr *ctlr) {
    certn_size_mem *mem = NULL;
    certn_size_mem_page *page = ctlr->page_head.next;
    certn_size_mem_page * const tail = (certn_size_mem_page*)&ctlr->page_tail;
    // 由于 unused_mem 块数为0的 page 会被移到尾部，
    // 所以如果 page->unused_mem_head==NULL 那么 link 中就没有可用的块了
    if (T1(page!=tail && page->unused_mem_head)) {
        mem = page->unused_mem_head;
        page->unused_mem_head = mem->next;
        page->mark++;
        // 把 unused_mem 块数为0的 page 移到尾部
        if (page->unused_mem_head==NULL) {
            certn_size_mem_page *last = page->last, *next = page->next;
            last->next = next;
            next->last = last;

            last = tail->last;
            page->next = tail;
            page->last = last;
            
            last->next = page;
            tail->last = page;
        }
    }
    return mem;
}

/// 释放 ctlr 占据的全部 page 的内存给操作系统
void certn_size_mem_ctlr_free_all_mem_unlock(certn_size_mem_ctlr *ctlr) {
    certn_size_mem_page * const tail = (certn_size_mem_page*)&ctlr->page_tail;
    certn_size_mem_page *page = (certn_size_mem_page*)ctlr->page_head.next, *temp;
    while (page!=tail) {
        temp = page;
        page = page->next;
        rmMem(temp, temp->page_size);
    }
    certn_size_mem_ctlr_init(ctlr, ctlr->mem_size);
}

//MARK: - 实现对外暴露的函数
/// ctlr 初始化为 mem_size 大小内存的管理者
void certn_size_mem_ctlr_init(certn_size_mem_ctlr *ctlr, size_t mem_size) {
    ctlr->hot_page = NULL;
    ctlr->page_head.last = ctlr->page_tail.next = NULL;
    ctlr->page_head.next = (certn_size_mem_page*)(&ctlr->page_tail);
    ctlr->page_tail.last = (certn_size_mem_page*)(&ctlr->page_head);
    // 最少是一个 certn_size_mem 类型大小
    ctlr->mem_size = mem_size > sizeof(certn_size_mem) ? mem_size : sizeof(certn_size_mem);
    ctlr->total_page_cnt = ctlr->used_mem_cnt = 0;
    ctlr->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
}

/// 通过 ctlr 获取固定大小的内存
void* certn_size_mem_ctlr_alloc_mem(certn_size_mem_ctlr *ctlr) {
    pthread_mutex_lock(&ctlr->lock);
    // 先去 mem_link 获取内存
    certn_size_mem *mem = mem_ctlr_get_mem_form_mem_link(ctlr);
    if (mem==NULL) {
        // mem_link 没有内存，那么去 hot_page 获取
        certn_size_mem_page *hot_page = ctlr->hot_page;
        // 如果 hot_page 为空，或者 hot_page 满了，则重新开辟一个 page
        if (T0(!hot_page || (hot_page->brk + ctlr->mem_size > hot_page->page_size))) {
            // 指数级增长
            size_t page_cnt = ctlr->total_page_cnt/2 + 1;
            size_t min_page_cnt = ceil((ctlr->mem_size + sizeof(certn_size_mem_page)+0.0) / PAGE_SIZE);
            // 避免 ctlr->mem_size 过大，而 page_cnt 过小时产生 bug
            if (T0(page_cnt < min_page_cnt)) { page_cnt = min_page_cnt; }
            ctlr->hot_page = hot_page = mem_page_new(page_cnt);
            if (T0(hot_page==NULL)) {
                mem = NULL;
                goto certn_size_mem_ctlr_alloc_mem_end;
            }
            
            ctlr->total_page_cnt += page_cnt;
            // 把 hot_page 放到 mem_page_link 的尾部
            certn_size_mem_page * const tail = (certn_size_mem_page*)&ctlr->page_tail;
            certn_size_mem_page * const last = tail->last;
            hot_page->next = tail;
            hot_page->last = last;
            last->next = hot_page;
            tail->last = hot_page;
        }
        hot_page->mark += 1;
        mem = (certn_size_mem*)(hot_page->mem + hot_page->brk);
        hot_page->brk += ctlr->mem_size;
    }
    ctlr->used_mem_cnt++;
certn_size_mem_ctlr_alloc_mem_end:;
    pthread_mutex_unlock(&ctlr->lock);
    return mem;
}

/// 通过 ctlr 释放固定大小的内存
void certn_size_mem_ctlr_free_mem(certn_size_mem_ctlr *ctlr, void *mem) {
    pthread_mutex_lock(&ctlr->lock);
#warning "free 时确保 page_head 处不为nil，不然性能会差一些"
    ctlr->used_mem_cnt--;
    certn_size_mem_page *page = mem_ctlr_get_page_of_mem(ctlr, mem);
    // 待优化，下面这行代码，占有了绝大多数的运行时间，猜测原因是内存块过大时，cache 命中率过低
    ((certn_size_mem*)mem)->next = page->unused_mem_head;
    page->unused_mem_head = mem;
    
    page->mark--;
    // page->mark 为0 代表用户已经没有使用 page 中的内存，此时考虑把 page 归还操作系统
    if (T0(page->mark==0)) {
        // used_mem_cnt 为 0 代表用户已经归还全部的内存，此时应该把内存归还操作系统
        if (T0(ctlr->used_mem_cnt==0)) {
            certn_size_mem_ctlr_free_all_mem_unlock(ctlr);
            goto certn_size_mem_ctlr_free_mem_end;
        }
        // 将 page 归还给操作系统后，空闲空间的占比，分母 +1.0进行数据类型转换和避免除0错误
        double rate = (ctlr->used_mem_cnt * ctlr->mem_size)/(ctlr->total_page_cnt * PAGE_SIZE - page->page_size + 1.0);
        
        if (rate < 0.77) {
            certn_size_mem_page *last = page->last, *next = page->next;
            if (page==ctlr->hot_page) {
                ctlr->hot_page = NULL;
            }
            last->next = next;
            next->last = last;
            ctlr->total_page_cnt -= (page->page_size)/PAGE_SIZE;
            rmMem(page, page->page_size);
        }
    }
certn_size_mem_ctlr_free_mem_end:;
    pthread_mutex_unlock(&ctlr->lock);
}

/// 释放 ctlr 占据的全部 page 的内存给操作系统
void certn_size_mem_ctlr_free_all_mem(certn_size_mem_ctlr *ctlr) {
    pthread_mutex_lock(&ctlr->lock);
    certn_size_mem_ctlr_free_all_mem_unlock(ctlr);
    pthread_mutex_unlock(&ctlr->lock);
}

void print_certn_size_mem_ctlr(certn_size_mem_ctlr const * const ctlr) {
    printf("hot_page = %p\n", ctlr->hot_page);
    printf("head.ln = %p %p, tail.ln = %p %p\n", ctlr->page_head.last, ctlr->page_head.next, ctlr->page_tail.last, ctlr->page_tail.next);
    printf("mem_size = %zu\n", ctlr->mem_size);
    printf("total_page_cnt = %zu, used_mem_cnt = %zu\n", ctlr->total_page_cnt, ctlr->used_mem_cnt);
    printf("\n\n");
}

