<<<<<<< HEAD
#ifndef _MY_MALLOC_H
#define _MY_MALLOC_H
=======
>>>>>>> 6582f4d8ef960cadc9ff78d9061b34e416821a17
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

<<<<<<< HEAD
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

#endif
=======
struct metadata {
  size_t size;
  bool available; 
  struct metadata *next; 
} typedef Node;

void * mymalooc(size_t size);
void split(size_t size, Node * node);
void mergeNode(Node * next, Node * ls);
void removeNode(Node * node);
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);
>>>>>>> 6582f4d8ef960cadc9ff78d9061b34e416821a17
