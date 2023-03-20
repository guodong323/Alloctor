//
//  mem_leak_detection.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/6/4.
//

#ifndef mem_leak_detection_h
#define mem_leak_detection_h
#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
// 用来检测内存泄露
void *mld_getMem(size_t size);
int mld_rmMem(void *addr, size_t size);

// 当cmz_malloc 和 cmz_free 次数平衡时，可以拿这个函数来测试是否出现了内存泄露，
// 返回值为 0 代表未泄露
int is_leak(void);

#ifdef __cplusplus
}
#endif
#endif /* mem_leak_detection_h */
