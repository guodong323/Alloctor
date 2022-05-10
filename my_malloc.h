#ifndef MALLOC_TUTORIAL_MY_MALLOC_H
#define MALLOC_TUTORIAL_MY_MALLOC_H

#include <stddef.h>
#include <pthread.h>

typedef struct block_meta {
    struct block_meta *next;
    struct block_meta *prev;
    size_t size;
} Node;


void malloc(size_t);
void calloc(size_t,size_t);
void realloc(void *,size_t);
void free(void *);



#endif //MALLOC_TUTORIAL_MY_MALLOC_H
