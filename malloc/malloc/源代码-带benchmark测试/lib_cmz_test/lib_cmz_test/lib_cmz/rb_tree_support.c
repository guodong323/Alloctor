//
//  rb_tree_support.c
//  cmz_allocator
//
//  Designed by [cjh mgd zzx] on 2022/5/15.
//

#include "rb_tree_support.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include "certn_size_mem_ctlr.h"
#include "GlobalHeader.h"

#define RED 1
#define BLACK 0

static rb_node* rb_rotate_left(rb_node* node, rb_node* root);
static rb_node* rb_rotate_right(rb_node* node, rb_node* root);
static rb_node* rb_insert_rebalance(rb_node *node, rb_node *root);
static rb_node* rb_erase_rebalance(rb_node *node, rb_node *parent, rb_node *root);
static rb_node* rb_search_auxiliary(key_t key, rb_node* root, rb_node** save);

static size_t rb_mem_ctlr_init_token = 0;
static certn_size_mem_ctlr rb_mem_ctlr;

void rb_mem_ctlr_init(void) {
    certn_size_mem_ctlr_init(&rb_mem_ctlr, sizeof(rb_node));
}

rb_node* rb_node_new(key_t key, data_t data) {
    once(&rb_mem_ctlr_init_token, rb_mem_ctlr_init);
    rb_node *node = rb_node_new_node();
    if (node==NULL) { return NULL; }
    node->key = key;
    node->data = data;
    return node;
}

rb_node* rb_node_new_node(void) {
    once(&rb_mem_ctlr_init_token, rb_mem_ctlr_init);
    rb_node *node = certn_size_mem_ctlr_alloc_mem(&rb_mem_ctlr);
    if (node==NULL) { return NULL; }
    node->parent = node->left = node->right = NULL;
    mem_ctl_link *lk = &node->data;
    mem_ctl_link_init(lk);
    return node;
}

void rb_node_free(rb_node *node) {
    certn_size_mem_ctlr_free_mem(&rb_mem_ctlr, node);
}

rb_node* rb_search(rb_node* root, key_t key) {
    return rb_search_auxiliary(key, root, NULL);
}

rb_node* rb_search_most_close(rb_node* root, key_t key) {
    rb_node *node = root;
    while (node) {
        if (node->key < key) {
            node = node->right;
        }else if (node->key > key) {
            if (node->left==NULL || node->left->key < key) { return node; }
            node = node->left;
        }else {
            return node;
        }
    }
    return NULL;
}

rb_node* rb_insert_node(rb_node* root, rb_node *node) {
    rb_node *parent = NULL;
    key_t key = node->key;
    if (rb_search_auxiliary(key, root, &parent)) {
        return root;
    }
    node->parent = parent;
    node->left = node->right = NULL;
    node->color = RED;
 
    if (parent) {
        if (parent->key > key) {
            parent->left = node;
        } else {
            parent->right = node;
        }
    } else {
        root = node;
    }
 
    return rb_insert_rebalance(node, root);
}

rb_node* rb_insert(key_t key, data_t data, rb_node* root) {
    rb_node *parent = NULL, *node;
    
    if ((node = rb_search_auxiliary(key, root, &parent))) {
        return root;
    }
    
    node = rb_node_new(key, data);
    node->parent = parent;
    node->left = node->right = NULL;
    node->color = RED;
 
    if (parent) {
        if (parent->key > key) {
            parent->left = node;
        } else {
            parent->right = node;
        }
    } else {
        root = node;
    }
    return rb_insert_rebalance(node, root);
}

rb_node* rb_erase_node(rb_node *root, rb_node *node) {
    if (!node) {
        printf("node can not be null !\n");
        return root;
    }
    rb_node *child = NULL, *parent, *old, *left;
    color_t color;
 
    old = node;
 
    if (node->left && node->right) {
        node = node->right;
        while ((left = node->left) != NULL) {
            node = left;
        }
        child = node->right;
        parent = node->parent;
        color = node->color;
  
        if (child) {
            child->parent = parent;
        }
        if (parent) {
            if (parent->left == node) {
                parent->left = child;
            } else {
                parent->right = child;
            }
        } else {
            root = child;
        }
  
        if (node->parent == old) {
            parent = node;
        }
  
        node->parent = old->parent;
        node->color = old->color;
        node->right = old->right;
        node->left = old->left;
  
        if (old->parent) {
            if (old->parent->left == old) {
                old->parent->left = node;
            } else {
                old->parent->right = node;
            }
        } else {
            root = node;
        }
  
        old->left->parent = node;
        if (old->right) {
            old->right->parent = node;
        }
    } else {
        if (!node->left) {
            child = node->right;
        } else if (!node->right) {
            child = node->left;
        }
        parent = node->parent;
        color = node->color;
  
        if (child) {
            child->parent = parent;
        }
        if (parent) {
            if (parent->left == node) {
                parent->left = child;
            } else {
                parent->right = child;
            }
        } else {
            root = child;
        }
    }
    rb_node_free(old);
    if (color == BLACK) {
        root = rb_erase_rebalance(child, parent, root);
    }
    return root;

}


// 删除
rb_node* rb_erase(rb_node *root, key_t key) {
    rb_node *node;
    
    // 查找要删除的结点
    if (!(node = rb_search_auxiliary(key, root, NULL))) {
        printf("key %zu is not exist!\n", key);
        return root;
    }
    
    return rb_erase_node(root, node);
}

static rb_node* rb_rotate_left(rb_node* node, rb_node* root) {
    rb_node* right = node->right;
    if ((node->right = right->left)) {
        right->left->parent = node;
    }
    right->left = node;
    if ((right->parent = node->parent)) {
        if (node == node->parent->right) {
            node->parent->right = right;
        } else {
            node->parent->left = right;
        }
    } else {
        root = right;
    }
    node->parent = right;
    return root;
}

static rb_node* rb_rotate_right(rb_node* node, rb_node* root) {
    rb_node* left = node->left;
    if ((node->left = left->right)) {
        left->right->parent = node;
    }
    left->right = node;
    if ((left->parent = node->parent)) {
        if (node == node->parent->right) {
            node->parent->right = left;
        } else {
            node->parent->left = left;
        }
    } else {
        root = left;
    }
    node->parent = left;
    return root;
}

static rb_node* rb_insert_rebalance(rb_node *node, rb_node *root) {
    rb_node *parent, *gparent, *uncle, *tmp;
    
    while ((parent = node->parent) && parent->color == RED) {
        gparent = parent->parent;
        if (parent == gparent->left) {
            uncle = gparent->right;
            if (uncle && uncle->color == RED) {
                uncle->color = BLACK;
                parent->color = BLACK;
                gparent->color = RED;
                node = gparent;
            } else {
                if (parent->right == node) {
                    root = rb_rotate_left(parent, root);
                    tmp = parent;
                    parent = node;
                    node = tmp;
                }
                parent->color = BLACK;
                gparent->color = RED;
                root = rb_rotate_right(gparent, root);
            }
        } else {
            uncle = gparent->left;
            if (uncle && uncle->color == RED) {
                uncle->color = BLACK;
                parent->color = BLACK;
                gparent->color = RED;
                node = gparent;
            } else {
                if (parent->left == node) {
                    root = rb_rotate_right(parent, root);
                    tmp = parent;
                    parent = node;
                    node = tmp;
                }
                parent->color = BLACK;
                gparent->color = RED;
                root = rb_rotate_left(gparent, root);
            }
        }
    }
    root->color = BLACK;
    return root;
}

static rb_node* rb_erase_rebalance(rb_node *node, rb_node *parent, rb_node *root) {
    rb_node *other, *o_left, *o_right;
 
    while ((!node || node->color == BLACK) && node != root) {
        if (parent->left == node) {
            other = parent->right;
            if (other->color == RED) {
                other->color = BLACK;
                parent->color = RED;
                root = rb_rotate_left(parent, root);
                other = parent->right;
            }
            if ((!other->left || other->left->color == BLACK) &&
                (!other->right || other->right->color == BLACK)) {
                other->color = RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->right || other->right->color == BLACK) {
                    if ((o_left = other->left)) {
                        o_left->color = BLACK;
                    }
                    other->color = RED;
                    root = rb_rotate_right(other, root);
                    other = parent->right;
                }
                other->color = parent->color;
                parent->color = BLACK;
                if (other->right) {
                    other->right->color = BLACK;
                }
                root = rb_rotate_left(parent, root);
                node = root;
                break;
            }
        } else {
            other = parent->left;
            if (other->color == RED) {
                other->color = BLACK;
                parent->color = RED;
                root = rb_rotate_right(parent, root);
                other = parent->left;
            }
            if ((!other->left || other->left->color == BLACK) &&
                (!other->right || other->right->color == BLACK)) {
                other->color = RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->left || other->left->color == BLACK) {
                    if ((o_right = other->right)) {
                        o_right->color = BLACK;
                    }
                    other->color = RED;
                    root = rb_rotate_left(other, root);
                    other = parent->left;
                }
                other->color = parent->color;
                parent->color = BLACK;
                if (other->left) {
                    other->left->color = BLACK;
                }
                root = rb_rotate_right(parent, root);
                node = root;
                break;
            }
        }
    }
    
    if (node) {
        node->color = BLACK;
    }
    return root;
}

static rb_node* rb_search_auxiliary(key_t key, rb_node* root, rb_node** save) {
    rb_node *node = root, *parent = NULL;
    key_t node_key;
    
    while (node) {
        parent = node;
        node_key = node->key;
        if (key < node_key) {
            node = node->left;
        } else if (key > node_key) {
            node = node->right;
        } else {
            return node;
        }
    }
 
    if (save) {
        *save = parent;
    }
 
    return NULL;
}

