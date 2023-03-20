//
//  rb_tree_support.h
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#ifndef rb_tree_support_h
#define rb_tree_support_h
#include <stdio.h>
#include "mem_ctl_link.h"

int rb_tree_supportMain(void);

//typedef int data_t;
typedef mem_ctl_link data_t;
typedef int color_t;
#define key_t size_t

typedef struct rb_node_t {
    struct rb_node_t *left, *right, *parent;
    color_t color;
    // key 为 mem_ctl_link.blk_size
    key_t key;
    // mem_ctl_link
    data_t data;
} rb_node;

//MARK: - 创建与释放
rb_node* rb_node_new_node(void);
rb_node* rb_node_new(key_t key, data_t data);
void rb_node_free(rb_node *node);

//MARK: - 插入删除查找
rb_node* rb_insert_node(rb_node* root, rb_node *node);
rb_node* rb_erase_node(rb_node *root, rb_node *node);

//rb_node* rb_insert(key_t key, data_t data, rb_node* root);
rb_node* rb_erase(rb_node *root, key_t key);
//
rb_node* rb_search(rb_node* root, key_t key);
// 查找 key 恰好大于等于 key 的节点，找不到时返回 null
rb_node* rb_search_most_close(rb_node* root, key_t key);

#endif /* rb_tree_support_h */
