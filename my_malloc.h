#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _block{
  size_t size;
  struct _block *prev;
  struct _block *next;
} block;

#define block_size sizeof(block)

size_t is_8(size_t size);

block *add_new_block(size_t size);

void seperate_block(block *current,size_t size);

block *merge_block(block *current);

block *get_block(void *p);

void *ts_malloc_nolock(size_t size);

void ts_free_nolock(void *p);

