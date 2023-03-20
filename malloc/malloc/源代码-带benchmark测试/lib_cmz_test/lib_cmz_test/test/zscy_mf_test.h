//
//  zscy_mf_test.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/6/5.
//

#ifndef zscy_mf_test_h
#define zscy_mf_test_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
void iof(void);

enum mf_type {
    alc = 0, fre = 1
};

typedef struct {
    enum mf_type type;
    union {
        struct {
            void *p;
            size_t size;
        };
        size_t alc_idx;
    };
} zscy_mf_opt;
#define opts_len 940379
extern zscy_mf_opt opts[opts_len];

void init_zscy_opt(void);

#ifdef __cplusplus
}
#endif
#endif /* zscy_mf_test_h */
