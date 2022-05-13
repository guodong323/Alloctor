#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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