//
//  zscy_mf_test.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/6/5.
//

#include "zscy_mf_test.h"


typedef struct {
    size_t mem_size;
    size_t alc_idx;
    size_t free_idx;
} opt;
// idx -> [1, 940378]
// mem -> [16, 8704]


zscy_mf_opt opts[opts_len];

void init_zscy_opt(void) {
    iof();
}
void iof(void) {
    FILE *f = fopen("/Users/yamayama/Downloads/zscy_mf_info.txt", "r");
    opt o;
    do {
        fscanf(f, "{%zu, %zu, %zu}, ", &o.mem_size, &o.alc_idx, &o.free_idx);
        opts[o.alc_idx].type = alc;
        opts[o.alc_idx].size = o.mem_size;
        opts[o.free_idx].type = fre;
        opts[o.free_idx].alc_idx = o.alc_idx;
    } while (o.mem_size || o.alc_idx || o.free_idx);
    fclose(f);
}
