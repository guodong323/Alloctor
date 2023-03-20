//
//  mf_test.c
//  benchmarkTest
//
//  Designed by [cjh mgd zzx] on 2022/6/4.
//

#define mf_test_c
#include "mf_test.h"
#include <stdlib.h>
#include <time.h>

struct alc_opt {
    size_t mem_size;
    void *mem_ptr;
};

struct free_opt {
    alc_opt *alc;
    struct free_opt *next;
};

struct mf_opt {
    alc_opt alc;
    free_opt *free_link;
};

struct mf_opt_ary {
    size_t len;
    free_opt *free_opt_arr;
    struct mf_opt ary[0];
};




struct rlc_opt {
    alc_opt *alc;
    size_t add_size;
    struct rlc_opt * next;
};
struct mcf_opt {
    alc_opt alc;
    free_opt * free_link;
    rlc_opt * rlc_link;
};
struct mcf_opt_ary {
    size_t len;
    free_opt *free_opt_arr;
    rlc_opt *rlc_opt_arr;
    struct mcf_opt ary[0];
};

mf_opt_ary* mf_opt_ary_new(size_t mf_cnt, size_t max_mem_size, unsigned rand_seed) {
    mf_opt_ary *opt_ary = (mf_opt_ary*)malloc(sizeof(mf_opt_ary) + mf_cnt * sizeof(mf_opt));
    opt_ary->len = mf_cnt;
    opt_ary->free_opt_arr = (free_opt*)malloc(mf_cnt * sizeof(free_opt));
    
    mf_opt *ary = opt_ary->ary;
    free_opt *free_opt_arr = opt_ary->free_opt_arr;
    size_t free_opt_idx = 0;
    srand(rand_seed);
    for (size_t i = 0; i < mf_cnt; ++i) {
        ary[i].free_link = NULL;
    }
    
    for (size_t i = 0; i < mf_cnt; ++i) {
        alc_opt *alc = &ary[i].alc;
        alc->mem_size = rand() % max_mem_size;
        // offset 是偏移，代表 ary[i] 处的 malloc 在 ary[ary + i + offset] 处进行free
        // 所以将对应的 free_opt 拼接到 ary[ary + i + offset] 的free_link上
        size_t offset = rand() % (mf_cnt-i);
        mf_opt *target = (ary + i + offset);
        free_opt *opt = free_opt_arr + (free_opt_idx++);
        opt->alc = alc;
        opt->next =  target->free_link;
        target->free_link = opt;
    }
    return opt_ary;
}

// 获取malloc-calloc-free 测试数组
mcf_opt_ary* mcf_opt_ary_new(size_t mf_cnt, size_t max_mem_size, unsigned rand_seed) {
    mcf_opt_ary *opt_ary = (mcf_opt_ary*)malloc(sizeof(mcf_opt_ary) + mf_cnt * sizeof(mcf_opt));
    opt_ary->len = mf_cnt;
    opt_ary->free_opt_arr = (free_opt*)malloc(mf_cnt * sizeof(free_opt));
    opt_ary->rlc_opt_arr = (rlc_opt*)malloc(mf_cnt * sizeof(rlc_opt));
    
    mcf_opt *ary = opt_ary->ary;
    free_opt *free_opt_arr = opt_ary->free_opt_arr;
    rlc_opt *rlc_opt_arr = opt_ary->rlc_opt_arr;
    size_t idx = 0;
    srand(rand_seed);
    for (size_t i = 0; i < mf_cnt; ++i) {
        ary[i].free_link = NULL;
        ary[i].rlc_link = NULL;
    }
    
    for (size_t i = 0; i < mf_cnt; ++i) {
        alc_opt *alc = &ary[i].alc;
        alc->mem_size = rand() % max_mem_size;
        // offset 是偏移，代表 ary[i] 处的 malloc 在 ary[ary + i + offset] 处进行free
        // 所以将对应的 free_opt 拼接到 ary[ary + i + offset] 的free_link上
        size_t offset = rand() % (mf_cnt-i);
        mcf_opt *fre_target = (ary + i + offset);
        mcf_opt *rlc_target = (ary + i + offset/2);
        free_opt *f_opt = free_opt_arr + idx;
        rlc_opt *c_opt = rlc_opt_arr + idx;
        ++idx;
        
        c_opt->alc = alc;
        c_opt->next = rlc_target->rlc_link;
        rlc_target->rlc_link = c_opt;
        c_opt->add_size = rand() % max_mem_size;
        
        if (c_opt->add_size + alc->mem_size >= max_mem_size) {
            c_opt->add_size = max_mem_size - alc->mem_size;
        }
        
        f_opt->alc = alc;
        f_opt->next =  fre_target->free_link;
        fre_target->free_link = f_opt;
    }
    return opt_ary;
}

void mcf_opt_ary_delete(mcf_opt_ary *opt_ary) {
    free(opt_ary->rlc_opt_arr);
    free(opt_ary->free_opt_arr);
    free(opt_ary);
}

void mf_opt_ary_delete(mf_opt_ary *opt_ary) {
    free(opt_ary->free_opt_arr);
    free(opt_ary);
}
