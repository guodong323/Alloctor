**项目描述**
实现一个可靠、健壮的、实时的内存分配器，有在储存出现一位bit错误时纠正错误的能力

实现目标：首先实现基础的内存分配功能，再为其加上并发的能力，在此基础上增加内存池功能库，完善纠错功能后
对代码优化改进性能和实时性

#Allocator 设计思路V3.0

总体思路：

分层：

① 库函数实现层：

1. 通过红黑树获取 blk，对 blk 进行拆分、合并、处理 blk 的标记位，每一个 blk 都对应了一个特定大小的内存块

②mem_ctlr 层：

1. 管理红黑树和内存块链表，对 ② 提供支持。不和 blk 标记位进行交互

③红黑树层：

1. 管理内存块，红黑树的每一个节点都是一个链表，链表的每一个元素都是一个 blk。
2. 提供获取、存储内存块的功能

④固定内存大小分配器：

1. 对红黑树节点占用的内存进行管理



对外接口：

```c
// 分配 size 字节的空间
void* malloc(size_t size);
// 分配 count 个大小为 size 字节的空间，并把每一位初始化为 0
void* calloc(size_t count, size_t size);
// 回收 ptr 指向的空间
void free(void *ptr);
// 将 ptr 指向的空间扩展到 size 字节
void* realloc(void *ptr, size_t size);

```





# 固定内存大小的内存分配器

用到的数据结构

```c
typedef struct certn_size_mem {
    // 指向下一块可用的内存
    struct certn_size_mem *next;
} certn_size_mem;

typedef struct certn_size_mem_page {
    char mem[0];
    // next 和 last 使全部的 mem_page 构成一个链表
    struct certn_size_mem_page *next, *last;
    // 该 page 中未使用的块的链表
    certn_size_mem *unused_mem_head;
    // 该 page 总共占多少内存
    size_t page_size;
    // 该 page 已经分配过多少次内存了，当 mark 变为0时，可以考虑把该 page 归还操作系统
    size_t mark;
    // mem[brk] 指向的地址可以拿来分配
    size_t brk;
} certn_size_mem_page;

// mem_page 构成的链表的虚头尾节点
typedef struct certn_size_mem_page_vht {
    char mem[0];
    struct certn_size_mem_page *next, *last;
} certn_size_mem_page_vht;

typedef struct certn_size_mem_ctlr {
    // 满足 hot_page->brk <= hot_page->page_size，
    // 需要分配空间时，先去 mem_page 构成的链表获取内存，
    // 无法获取到内存时，去hot_page 获取。
    certn_size_mem_page *hot_page;
    // mem_page 构成的链表的虚头尾节点
    certn_size_mem_page_vht page_head, page_tail;
    // certn_size_mem_ctlr 分配出去的内存块的大小
    size_t mem_size;
    // 已经分配多少内存块出去了
    size_t used_mem_cnt;
    // 全部的 pages 总共占有了多少个 4KB 内存
    size_t total_page_cnt;
    // 锁，确保线程安全
    pthread_mutex_t lock;
} certn_size_mem_ctlr;

```

对外接口：

```c
/// ctlr 初始化为 mem_size 大小内存的管理者
void certn_size_mem_ctlr_init(certn_size_mem_ctlr *ctlr, size_t mem_size);
/// 通过 ctlr 获取固定大小的内存
void* certn_size_mem_ctlr_alloc_mem(certn_size_mem_ctlr *ctlr);
/// 通过 ctlr 释放固定大小的内存
void certn_size_mem_ctlr_free_mem(certn_size_mem_ctlr *ctlr, void *mem);
/// 释放 ctlr 占据的全部 page 的内存给操作系统
void certn_size_mem_ctlr_free_all_mem(certn_size_mem_ctlr *ctlr);
```



###### 实现的功能：

通过 certn_size_mem_ctlr 实现一个固定大小的内存分配器。初始化 certn_size_mem_ctlr 时，需要指定其分配出去的内存块的大小。初始化完后可以通过 certn_size_mem_ctlr_alloc_mem() 获取内存块，通过 certn_size_mem_ctlr_free_mem 释放内存块。支持在多线程环境下的内存管理。



###### 使用示例：

```c
typedef struct Link {
    int num;
    struct Link *next;
} Link;
// 创建长度为 100 的链表：
void demo(void) {
    certn_size_mem_ctlr mem_ctlr;
    certn_size_mem_ctlr_init(&mem_ctlr, sizeof(Link));
    Link *head, *temp;
    head = temp = (Link*)certn_size_mem_ctlr_alloc_mem(&mem_ctlr);
    for (int i = 0; i < 99; ++i) {
        // 使用 certn_size_mem_ctlr_alloc_mem 获取内存
        temp->next = (Link*)certn_size_mem_ctlr_alloc_mem(&mem_ctlr);
        temp = temp->next;
    }
    temp->next = NULL;
    // 链表创建完毕
    // 下面释放链表空间
    while (head) {
	    temp = head;
        head = head->next;
        // 使用 certn_size_mem_ctlr_alloc_mem 释放内存
        certn_size_mem_ctlr_free_mem(&mem_ctlr, temp);
    }
}

```

###### 实现思路：

certn_size_mem_ctlr_alloc_mem 思路：

先去 certn_size_mem_ctlr 的管理的 certn_size_mem_page 链表的第一个page 的空闲内存块链表获取内存块，如果该内存块是 空闲块链表的最后一块内存，那么将该 page 移到 page 链表的尾部。如果获取不到内存块，那么去 hot_page 通过 brk 获取内存，如果 hot_page 没有空闲空间了，那么通过mmap重新开辟一块内存，从该块内存获取内存块，并将该内存块设置为 hot_page.

certn_size_mem_ctlr 即相关关键数据结构和成员变量的组织关系：

![截屏2022-06-05 20.40.46](/Users/yamayama/Downloads/文档/picture/截屏2022-06-05 20.40.46.png)



certn_size_mem_ctlr_alloc_mem的大致流程图（省略了一些边界情况）

![截屏2022-06-05 20.38.08](/Users/yamayama/Downloads/文档/picture/截屏2022-06-05 20.38.08.png)

certn_size_mem_ctlr_free_mem 大致流程：

![截屏2022-06-05 21.00.33](/Users/yamayama/Downloads/文档/picture/截屏2022-06-05 21.00.33.png)





设计思路：

使用 mmap 系统调用来获取大块的内存，在对大内存块进行管理

用红黑树管理内存块：
key为内存块大小，val为内存块元数据链表，链表内元数据管理的内存大小一致。

无论是正在使用中，或者是空闲的内存块，都需要有一个内存块控制单元，对其进行管理，方便内存的分配与回收，其结构设计如下：

由相同大小的内存块构成的内存块链表结构：

```c
// mem_ctl_blk 的标记位
typedef union mem_ctl_blk_flags {
    size_t bits;
    struct {
        // 标记 blk 是否正空闲，这个标记位的修改规则是：谁直接改变 sibling 链，谁修改
        size_t is_free : 1;
        // 标记 blk 是否是 sibling_head
        size_t is_sibling_head : 1;
		// 标记 blk 是否是 sibling_tail
        size_t is_sibling_tail : 1;
        // 谁改变 addr 链，谁修改
        size_t is_addr_head : 1;
        size_t is_addr_tail : 1;
    };
} mem_ctl_blk_flags;

typedef struct mem_ctl_blk {
    // 标记位
    mem_ctl_blk_flags flags;
    // addr_last、addr_next 指向内存地址相连续的上一块、下一块、free 时要用，使用虚头尾机制
    struct mem_ctl_blk *addr_last, *addr_next;
    // align 用来占位，使 mem_ctl_blk 占用的空间对 MIN_MEM_SIZE 对齐
    size_t align[0];
    // 从 data 指向的空间起，都是拿来分配出去的
    char mem_start[0];
    // 指向[块大小]一样的上一块，下一块，在红黑树中的节点才会用到
    struct mem_ctl_blk *sibling_last, *sibling_next;
} mem_ctl_blk;


// 组织一个链表，链表内的所有内存块的空间大小都一样
typedef struct mem_ctl_link {
    // head->sibling_next == tail 代表 mem_ctl_link 为空，此时需要从红黑树移除这个 link
    mem_ctl_blk head, tail;
    // block 的大小 存在 rb_node_t
} mem_ctl_link;



```





内存控制器：管理红黑树和内存块链表，对 malloc 提供支持。
创建的 blk 才会被存进该 哈希表，mem_ctl_store_blk 内部会进行空闲块的合并，在恰当时机把 blk 归还给操作系统。统计分配出去的空间、红黑树可用的空间，当 blk 可以被归还时，尽量使 分配出去的空间 *0.4 <= 红黑树可用的空间  <= 分配出去的空间 *0.6（这个参数是凭感觉定的，后面可以再改）。当 malloc/calloc 和 free 平衡时，应当把空间全部归还。

```c
struct mem_ctlr {
    rb_node_t *rb_root;
} mem_ctlr;

提供接口：
// 从红黑树获取[内存大小] >= size 的 blk，保证返回值不为空
mem_ctl_blk* mem_ctl_get_blk(mem_ctlr *ctlr, size_t size);
// 实现空闲块的合并与放回红黑树、在恰当时机将空闲块归还操作系统
void mem_ctl_store_blk(mem_ctlr *ctlr, mem_ctl_blk* blk);
```



malloc 设计思路：
调用 malloc(size) 内部：去内部维护的一个红黑树上去寻找一个 key 最接近 size+sizeof(mem_ctl_blk) 的节点。如果找到了该节点，则从该节点代表的的内存块链表中取出一块内存，如果取出一块内存后，该链表长度变为0，则将该节点从红黑树移除。对取出的内存块进行分割，一部分分配出去，给malloc的调用者使用，另一部分放回红黑树，等待下次使用。

malloc 伪代码：

```c
static mem_ctlr ctl;
void* malloc(size_t size) {
	if (size==0) { return NULL; }
//    const size_t new_size = size + sizeof(mem_ctl_blk);
    mem_ctl_blk *blk = mem_ctlr_get_blk(&ctlr, size);
    if (mem_ctl_blk_get_data_size(blk) > size + sizeof(mem_ctl_blk)) {
        // 多出来的块
        mem_ctl_blk *ret_blk = (mem_ctl_blk*)(blk->data + size);
        // 更新两个的内存块的链接关系
        if ((ret_blk->addr_next = blk->addr_next)) {
            blk->addr_next->addr_last = ret_blk;
        }
        ret_blk->addr_last = blk;
        blk->addr_next = ret_blk;
        mem_ctlr_store_blk(&ctlr, ret_blk);
    }
    return blk->data;
}
```



free 设计思路：

调用 free(addr) 内部：通过对addr进行地址偏移，获得内存块控制单元地址，通过控制单元内部的控制信息，进行内存回收、空闲空间的合并

伪代码：

```c
void free(void* addr) {
    mem_ctl_blk *blk = mem_ctl_blk_from_data(ptr);
    mem_ctlr_store_blk(&ctlr, blk);
}
```



calloc 设计思路：

calloc 内部调用 malloc 获取空间，再把获取的空间中的每一位全部置0



realloc 设计思路：

调用 realloc(void *addr, size_t size) 内部：获取到内存控制块后，检查 blk->addr_next 是否在使用，空间是否满足扩容的需求，如果满足，则从 blk->addr_next 获取额外的空间。如果不满足，则调用malloc获取一个大小为 size 的内存块，把旧内存块的数据转移到新内存块，再把新内存块释放。

伪代码：

```c
void* realloc(void *addr, size_t size) {
	// 通过地址偏移，获得控制单元
    mem_ctl_blk *blk = mem_ctl_blk_from_data(ptr);
    size_t ext_size = size - mem_ctl_blk_get_data_size(blk);
    mem_ctl_blk *next_blk = blk->addr_next;
    if (mem_ctl_blk_is_free(next_blk)) {
        // 如果下一个
        if (mem_ctl_blk_get_data_size(next_blk) >= ext_size) {
            // 内部完成链表的断开工作，从红黑树移除
            mem_ctl_blk_remove_from_sibling_link(next_blk);
            // 分割
            mem_ctl_blk *new_next_blk = (mem_ctl_blk*)(((char*)next_blk) + ext_size);
            // 处理链接关系
            blk->addr_next = new_next_blk;
            new_next_blk->addr_last = blk;
            if ((new_next_blk->addr_next = next_blk->addr_next)) {
                next_blk->addr_next->addr_last = new_next_blk;
            }
            // 将blk->addr_next 放回红黑树
            mem_ctlr_store_blk(&ctlr, new_next_blk);
        }else if (mem_ctl_blk_get_size(next_blk) >= ext_size) {
            mem_ctl_blk_remove_from_sibling_link(next_blk);
            if ((blk->addr_next = next_blk->addr_next)) {
                next_blk->addr_next->addr_last = blk;
            }
        }else {
            goto malloc_new_space;
        }
        return blk;
    }
malloc_new_space:;
    void *new_space = malloc(size);
    memmove(new_space, ptr, mem_ctl_blk_get_data_size(blk));
    free(ptr);
    return new_space;
}

**进程**
1. 内存分配器编写完毕，测试无问题，未出现内存泄露
2. 锁机制处于编写阶段
3. 内存池库处于设计阶段
4. 明确纠错方法，未实践


遇到的问题：relloc出现bug  原因是内存块长度过小时会出现覆盖头部导致信息丢失。（现已解决）
```
