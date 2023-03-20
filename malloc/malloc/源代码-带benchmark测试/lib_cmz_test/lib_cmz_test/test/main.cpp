//
//  main.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/11.
//

#include <iostream>
#include <benchmark/benchmark.h>
#include <cmath>
#include "mf_test.h"
#include "lib_cmz.h"
#include "GlobalHeader.h"
#include "zscy_mf_test.h"

typedef struct mem_ctl_func {
    // 分配 size 字节的空间
    void* (*malloc)(size_t size);
    // 分配 count 个大小为 size 字节的空间，并把每一位初始化为 0
    void* (*calloc)(size_t count, size_t size);
    // 回收 ptr 指向的空间
    void (*free)(void *ptr);
    // 将 ptr 指向的空间扩展到 size 字节
    void* (*realloc)(void *ptr, size_t size);
} mem_ctl_func;

enum mf_fun_type {
    cmz = 0,    // 代表使用 cmz_malloc 进行测试
    sys = 1     // 代表使用 系统 malloc 进行测试
};

const mem_ctl_func func_ary[2] = {
    { cmz_malloc, cmz_calloc, cmz_free, cmz_realloc},
    { malloc, calloc, free, realloc},
};

// 用于产生随机大小内存的 malloc-free 测试
void mf_args_creator(benchmark::internal::Benchmark* b) {
    srand(0);
    const unsigned len = 1 << 5;
    long long rand_seed_ary[len];
    for (size_t i = 0; i < len; ++i) {
        rand_seed_ary[i] = rand() % (len * 10);
    }
    b->ArgNames({" type", " cnt", " size", " seed"})->Name("test");
    // max_mc: malloc-free 的最大次数，min_ms:最小内存块大小，max_ms:最大内存块大小
    auto f = [&](long long max_mc, long long min_ms, long long max_ms)->void {
        int rand_seed_idx = 0;
        for (long long mf_cnt = 2; mf_cnt <= max_mc; ++mf_cnt) {
            for (long long ms = min_ms; ms <= max_ms; ++ms) {
                rand_seed_idx = (rand_seed_idx + 1) & (len-1);
                long long rand_seed = rand_seed_ary[rand_seed_idx];
                b->Args({cmz, (long long)pow(10, mf_cnt), 1l<<ms, rand_seed});
                b->Args({sys, (long long)pow(10, mf_cnt), 1l<<ms, rand_seed});
            }
        }
    };
    
    // 小内存块，测试 (1e2) * (1<<3 B) 到 (1e7)*(1<<10 B) 的 malloc - free
    f(7, 3, 10);
    // 大内存块，测试 (1e2) * (1<<11 B) 到 (1e6)*(1<<20 B) 的 malloc - free
    f(6, 11, 20);
    
    // 1e7 * 2^10 -> 2^33
    // 1e6 * 2^20 -> 2^40
}

// 随机大小内存的 malloc-free 测试
static void bm_mf_test(benchmark::State& state) {
    // Perform setup here
    // malloc 类型
    size_t type = state.range(0);
    // malloc-free 次数
    size_t mf_cnt = state.range(1);
    // 最大的内存块大小
    size_t max_mem_size = state.range(2);
    // 随机数种子
    unsigned rand_seed = (unsigned)state.range(3);
    const mem_ctl_func * const funcs = func_ary + type;
    // 获取一个malloc - free 测试数组
    mf_opt_ary *opt_ary = mf_opt_ary_new(mf_cnt, max_mem_size, rand_seed);
    
    for (auto _ : state) {
        // This code gets timed
        size_t len = opt_ary->len;
        mf_opt *ary = opt_ary->ary;
        for (size_t i = 0; i < len; ++i) {
            ary[i].alc.mem_ptr = funcs->malloc(ary[i].alc.mem_size);
            free_opt *flk = ary[i].free_link;
            if (ary[i].alc.mem_ptr==NULL) {
                printf("memory run out!\n");
            }
            while (flk) {
                funcs->free(flk->alc->mem_ptr);
                flk = flk->next;
            }
        }
    }
    // 内存泄露检测函数，检测 cmz 内存分配器的内存泄露情况
    assert(is_leak()==0);
    mf_opt_ary_delete(opt_ary);
}

// 用于产生随机大小内存的 malloc-calloc-free 测试
void mcf_args_creator(benchmark::internal::Benchmark* b) {
    srand(0);
    const unsigned len = 1 << 5;
    long long rand_seed_ary[len];
    for (size_t i = 0; i < len; ++i) {
        rand_seed_ary[i] = rand() % (len * 10);
    }
    b->ArgNames({" type", " cnt", " size", " seed"})->Name("test");
    
    // max_mc: malloc-free 的最大次数，min_ms:最小内存块大小，max_ms:最大内存块大小
    auto f = [&](long long max_mc, long long min_ms, long long max_ms)->void {
        int rand_seed_idx = 0;
        for (long long mf_cnt = 2; mf_cnt <= max_mc; ++mf_cnt) {
            for (long long ms = min_ms; ms <= max_ms; ++ms) {
                rand_seed_idx = (rand_seed_idx + 1) & (len-1);
                long long rand_seed = rand_seed_ary[rand_seed_idx];
                b->Args({cmz, (long long)pow(10, mf_cnt), 1l<<ms, rand_seed});
                b->Args({sys, (long long)pow(10, mf_cnt), 1l<<ms, rand_seed});
            }
        }
    };
    
    // 小内存块，测试 (1e2) * (1<<3 B) 到 (1e6)*(1<<10 B) 的 malloc - free
    f(6, 3, 10);
    // 大内存块，测试 (1e2) * (1<<11 B) 到 (1e5)*(1<<20 B) 的 malloc - free
    f(5, 11, 20);
    
    // 1e7 * 2^10 -> 2^33
    // 1e6 * 2^20 -> 2^40
}

// 随机大小内存的 malloc-calloc-free 测试
static void bm_mcf_test(benchmark::State& state) {
    // Perform setup here
    // malloc 类型
    size_t type = state.range(0);
    // malloc-free 次数
    size_t mf_cnt = state.range(1);
    // 最大的内存块大小
    size_t max_mem_size = state.range(2);
    // 随机数种子
    unsigned rand_seed = (unsigned)state.range(3);
    const mem_ctl_func * const funcs = func_ary + type;
    // 获取一个malloc - free 测试数组
    mcf_opt_ary *opt_ary = mcf_opt_ary_new(mf_cnt, max_mem_size, rand_seed);
    
    for (auto _ : state) {
        // This code gets timed
        size_t len = opt_ary->len;
        mcf_opt *ary = opt_ary->ary;
        for (size_t i = 0; i < len; ++i) {
            ary[i].alc.mem_ptr = funcs->malloc(ary[i].alc.mem_size);
            if (ary[i].alc.mem_ptr==NULL) {
                printf("memory run out!\n");
            }
            // realloc
            rlc_opt *rlk = ary[i].rlc_link;
            if (rlk) {
                alc_opt *alc = rlk->alc;
                alc->mem_ptr = (void*)(funcs->realloc(alc->mem_ptr, alc->mem_size + rlk->add_size));
                if (alc->mem_ptr==NULL) {
                    printf("memory run out!\n");
                }
                *((size_t*)&alc->mem_size) = alc->mem_size + rlk->add_size;
                rlk = rlk->next;
            }
            
            // free
            free_opt *flk = ary[i].free_link;
            while (flk) {
                funcs->free(flk->alc->mem_ptr);
                flk = flk->next;
            }
        }
    }
    // 内存泄露检测函数，检测 cmz 内存分配器的内存泄露情况
    assert(is_leak()==0);
    mcf_opt_ary_delete(opt_ary);
}

// 使用 “掌上重邮APP” 的 malloc-free 数据进行测试
static void zscy_mf_test(benchmark::State& state) {
    size_t type = state.range(0);
    const mem_ctl_func * const funcs = func_ary + type;
    for (auto _ : state) {
        for (size_t i = 1; i < opts_len; ++i) {
            if (opts[i].type==alc) {
                opts[i].p = funcs->malloc(opts[i].size);
                if (opts[i].p==NULL) {
                    printf("memory run out!\n");
                }
            }else {
                funcs->free(opts[opts[i].alc_idx].p);
            }
        }
    }
    // 内存泄露检测函数，检测 cmz 内存分配器的内存泄露情况
    assert(is_leak()==0);
}

// 使用随机函数随机地进行 malloc-free 测试
//BENCHMARK(bm_mf_test)->Apply(mf_args_creator);
//// 使用随机函数随机地进行 malloc-calloc-free 测试
//BENCHMARK(bm_mcf_test)->Apply(mcf_args_creator);

// 使用 “掌上重邮APP” 的 malloc-free 数据进行测试，需要在 init 函数中调用 init_zscy_opt();
// 还需要去修改 “zscy_mf_test.c” 的 “iof” 函数的文件路径
BENCHMARK(zscy_mf_test)->Arg(0)->Arg(1)->Iterations(20);


// 全局初始化
void init(void) {
    printf("init\n");
    init_zscy_opt();
}

// 全局析构
void deinit(void) {
    printf("deinit\n");
}

// Run the benchmark
int main(int argc, char** argv) {
    init();
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    deinit();
    return 0;
}

