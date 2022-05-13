
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "my_malloc.h"
#include <stdbool.h>
#include "pthread.h"
#define block_size sizeof(block)

pthread_mutex_t lock =  PTHREAD_MUTEX_INITIALIZER;

size_t is_8(size_t size){
  if(size % 8 == 0) return size;
  return ((size>>3)+1)<<3;
}

__thread void *first_block_nolock = NULL;

block *bf_block_nolock(size_t size){
    block *current  = first_block_nolock;
    block *bf_block = NULL;
    while(current != NULL) {
        // 符合要求的块
        if(current->size >= size) {
        if(bf_block == NULL || current->size < bf_block->size){
            bf_block = current;
            // 找到大小正好相等的就是最匹配的
            if(current->size == size) {
                break;
            }
        }
        }
        current = current->next;
    }
    // 找不到就没有
    if(bf_block == NULL) return NULL;
    // 找到的block比要分配的空间还大，就分割
    if(bf_block->size > 8 + size + block_size) {
        seperate_block(bf_block,size);
    }
    // 如果是头节点
    if(bf_block == first_block_nolock){
        first_block_nolock = bf_block->next;
        if(bf_block->next != NULL){
            bf_block->next->prev = NULL;
        }
    } else{
        bf_block->prev->next = bf_block->next;
        if(bf_block->next != NULL){
            bf_block->next->prev = bf_block->prev;
        }
    }
    return bf_block;
}

// 新申请空间
block *add_new_block_nolock(size_t size){
  pthread_mutex_lock(&lock);
  block *new_block = sbrk(size+block_size);          
  pthread_mutex_unlock(&lock);
  new_block->size = size;
  new_block->prev = NULL;
  new_block->next = NULL;
  return new_block;
}

// 分割空间
void seperate_block(block *current,size_t size){
    block *new_block;
    new_block = (block*)((size_t)current+ size + block_size);
    // 分割出要用的
    new_block->size = current->size - size - block_size;
    new_block->next = current->next;
    new_block->prev = current;
    current->size = size;
    current->next = new_block;
    // 加到free list中去
    if(new_block->next != NULL){
        new_block->next->prev = new_block;
    }
}

block *merge_block(block *current){
    if(current->next != NULL && ((size_t)current + current->size + block_size == (size_t)(current->next))) {
        current->size += current->next->size + block_size;
        current->next = current->next->next;
        if(current->next){
            current->next->prev = current;
        }
    }
    return current;
}

// 判断是否合法
block *get_block(void *p){
    size_t temp = (size_t)p - block_size;
    return (block *)temp;
}

void *ts_malloc_nolock(size_t size){
    if(size <= 0) return NULL;
    if(first_block_nolock == NULL) {
        pthread_mutex_lock(&lock);
        first_block_nolock = sbrk(block_size);
        pthread_mutex_unlock(&lock);
    }
    block *target = NULL;
    size_t s = is_8(size);
    if(first_block_nolock != NULL) {
        target = bf_block_nolock(s);
        if(target == NULL){
            target = add_new_block_nolock(s);
            if(target == NULL) return NULL;
        }
    } else{
        target = add_new_block_nolock(s);
        if(target == NULL) return NULL;
    }
    return (void *)((size_t)target+block_size);
}

void ts_free_nolock(void *p){
    if(p==NULL) return;
    block *current;
    block *free_block;
    free_block = get_block(p);
    current = first_block_nolock;
    if(current == NULL){
        first_block_nolock = free_block;
        free_block->next = NULL;
        free_block->prev = NULL;
        return;
    }
    while(current != NULL && current->next != NULL && (size_t)free_block > (size_t)current){
        current = current->next;
    }

    if((size_t)free_block < (size_t)current) {
        if(current == first_block_nolock) {
            first_block_nolock = free_block;
            free_block->prev = NULL;
            free_block->next = current;
            if(current!=NULL) {
            current->prev = free_block;
            }
        } else {
            free_block->next =  current;
            free_block->prev = current->prev;
            current->prev->next = free_block;
            current->prev = free_block;
        }
    } else if((size_t)free_block>(size_t)current){
        current->next = free_block;
        free_block->prev = current;
        free_block->next = NULL;
    }

    if(free_block->prev && ((size_t)free_block->prev+block_size+(free_block->prev->size) == (size_t)free_block)){
        free_block = merge_block(free_block->prev);
    }
    if(free_block->next && ((size_t)free_block+block_size+free_block->size == (size_t)free_block->next)) free_block = merge_block(free_block);

}
