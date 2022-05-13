#include "my_malloc.h"

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

__thread Node *initialTLS = NULL;  //每个线程的头
static Node *initialFree = NULL;   //总共的头

pthread_mutex_t sbrk_locker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

// 分配内存空间
void* mymalloc(size_t size) {
  pthread_mutex_lock(&sbrk_locker);
  Node * ls = (Node *)sbrk(0);
  size_t realSize = size + sizeof(Node);
  if (sbrk(realSize) == (void *)-1) {  
    pthread_mutex_unlock(&sbrk_locker);
    return NULL;
  }
  pthread_mutex_unlock(&sbrk_locker);
  ls->size = size;
  ls->available = false;
  ls->next = NULL;
  return ls;
}

// 空间太大了就拆开
void split(size_t size, Node * node) {
  size_t tmpSize = node->size;
  node->size = (tmpSize - size - sizeof(Node));
  Node * splitted = (Node *)((char *)node + node->size + sizeof(Node));
  splitted->size = size;
  splitted->next = NULL;
}

// 合并起来
void mergeNode(Node * cur, Node * ls) {
  Node * next = cur->next;
  if (initialTLS == next) {
    initialTLS = ls;
  }
  ls->next = next->next;
  cur->next = ls;
  ls->size = ls->size + next->size + sizeof(Node);
  ls->available = true;
  next->available = false;
  next->next = NULL;
}

//从freelist中删除
void removeNode(Node *node) {
  Node * toDelete = node->next;
  toDelete->available = false;
  node->next = toDelete->next;

  if (initialTLS == toDelete) {
    initialTLS = toDelete->next;
  }
  toDelete->next = NULL;
}

void * ts_malloc_nolock(size_t size) {
  Node * tmp = initialTLS;
  int cnt = 0;
  int fit = 0;
  size_t minSize = INT_MAX;
  while (tmp != NULL && tmp->next != NULL) {
    if (tmp->next->size == size) {
      Node * toDelete = tmp->next;
      removeNode(tmp);
      return (char *)toDelete + sizeof(Node);
    }
    else if (tmp->next->size > size && minSize > tmp->next->size) {
      minSize = tmp->next->size;
      fit = cnt;
    }
    cnt++;
    tmp = tmp->next;
  }

  if (minSize == INT_MAX) {
    return (char *)mymalloc(size) + sizeof(Node);
  }

  Node * tmp2 = initialTLS;
  for (int i = 0; i < fit; i++) {
    tmp2 = tmp2->next;
  }
  Node * ans = tmp2;
  if (tmp2->next->size > 1 * (size + sizeof(Node))) {
    split(size, tmp2->next);
    ans = (Node *)((char *)tmp2->next + tmp2->next->size + sizeof(Node));
  }
  else {
    ans = tmp2->next;
    removeNode(tmp2);
  }
  return (char *)ans + sizeof(Node);
}

//free
void ts_free_nolock(void * ptr) {
  Node * ls = (Node *)((char *)ptr - sizeof(Node));
  if (ls->available == true) {
    return;
  }

  Node * next = (Node *)((char *)ls + ls->size + sizeof(Node));

  Node * cur = initialTLS;
  if (next < (Node *)sbrk(0) && next->available == true && next != initialTLS) {
    while (cur != NULL && cur->next != NULL && cur->next != next) {
      cur = cur->next;
    }
    if (ls != NULL && cur != NULL && cur->next != NULL) {
      mergeNode(cur, ls);
    }
    else {
      ls->available = true;
      ls->next = initialTLS;
      initialTLS = ls;
    }
  }
  else {
    ls->available = true;
    ls->next = initialTLS;
    initialTLS = ls;
  }
}