//
//  mf_test.h
//  benchmarkTest
//
//  Designed by [cjh mgd zzx] on 2022/6/4.
//

#ifndef mf_test_h
#define mf_test_h
#ifdef __cplusplus
extern "C" {
#endif




#include <stdio.h>
typedef struct alc_opt alc_opt;
typedef struct free_opt free_opt;
typedef struct mf_opt mf_opt;
typedef struct mf_opt_ary mf_opt_ary;


typedef struct rlc_opt rlc_opt;
typedef struct mcf_opt mcf_opt;
typedef struct mcf_opt_ary mcf_opt_ary;

#ifndef mf_test_c

struct alc_opt {
    const size_t mem_size;
    void *mem_ptr;
};

struct free_opt {
    const alc_opt *alc;
    struct free_opt * const next;
};

struct mf_opt {
    alc_opt alc;
    free_opt * const free_link;
};

struct mf_opt_ary {
    const size_t len;
    const free_opt *free_opt_arr;
    struct mf_opt ary[0];
};




struct rlc_opt {
    alc_opt *alc;
    const size_t add_size;
    struct rlc_opt * const next;
};
struct mcf_opt {
    alc_opt alc;
    free_opt * const free_link;
    rlc_opt * const rlc_link;
};
struct mcf_opt_ary {
    const size_t len;
    const free_opt *free_opt_arr;
    const rlc_opt *rlc_opt_arr;
    struct mcf_opt ary[0];
};

#endif

// 获取malloc-free 测试数组
mf_opt_ary* mf_opt_ary_new(size_t mf_cnt, size_t max_mem_size, unsigned rand_seed);

void mf_opt_ary_delete(mf_opt_ary *opt_ary);

// 获取malloc-calloc-free 测试数组
mcf_opt_ary* mcf_opt_ary_new(size_t mf_cnt, size_t max_mem_size, unsigned rand_seed);
void mcf_opt_ary_delete(mcf_opt_ary *opt_ary);


#ifdef __cplusplus
}
#endif
#endif /* mf_test_h */
