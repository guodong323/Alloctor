//
//  lib_cmz.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/13.
//

#ifndef lib_cmz_h
#define lib_cmz_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

// 分配 size 字节的空间
void* cmz_malloc(size_t size);
// 分配 count 个大小为 size 字节的空间，并把每一位初始化为 0
void* cmz_calloc(size_t count, size_t size);
// 回收 ptr 指向的空间
void cmz_free(void *ptr);
// 将 ptr 指向的空间扩展到 size 字节
void* cmz_realloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* lib_cmz_h */


/*
 // 分配 size 字节的空间
 void* malloc(size_t size);
 // 分配 count 个大小为 size 字节的空间，并把每一位初始化为 0
 void* calloc(size_t count, size_t size);
 // 回收 ptr 指向的空间
 void free(void *ptr);
 // 将 ptr 指向的空间扩展到 size 字节
 void* realloc(void *ptr, size_t size);
 */
