//
//  GlobalHeader.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/13.
//

#include "GlobalHeader.h"
#include <pthread.h>

/// 将 size 对齐到 n 的倍数，要求 n 为 2的n次方
size_t alignN(size_t size, size_t n) {
    return ((size + n - (size!=0)) & (-n));
}

void once(size_t *token, void(*f)(void)) {
    if (T0(*token==0)) {
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        // 获取修改权
        pthread_mutex_lock(&lock);
        if (T0(*token==0)) {
            // 下面3行次序不能变，不然当 f 里面调用 once 时可能导致死锁
            ++(*token);
            pthread_mutex_unlock(&lock);
            if (f) { f(); }
        }else {
            pthread_mutex_unlock(&lock);
        }
    }
}

//void once1(size_t *token, void(*f)(void)) {
//    if (*token==0) {
//        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//        // 获取修改权
//        pthread_mutex_lock(&lock);
//        if (*token==0) {
//            // 下面3行次序不能变，不然当 f 里面调用 once 时可能导致死锁
//            ++(*token);
//            pthread_mutex_unlock(&lock);
//            if (f) { f(); }
//        }else {
//            pthread_mutex_unlock(&lock);
//        }
//    }
//}

