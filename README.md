**新版文档在提交文件中已更新**
***proj28 内存分配器***





# Allocator 设计思路




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



设计思路：

用红黑树管理内存块：
key为内存块大小，val为内存块元数据链表，链表内元数据管理的内存大小为一定范围。

无论是正在使用中，或者是空闲的内存块，都需要有一个内存块控制单元，对其进行管理，方便内存的分配与回收，其结构设计如下：

```c
typedef struct mem_ctl_blk {
    // addr_next、addr_last 指向内存地址相连续的下一块、上一块、free 时使用
    struct mem_ctl_blk *addr_next, *addr_last;
    // 指向[块大小]一样的下一块，如果正在被使用或者为虚头尾节点，那么为nil，该数据段用来构成 mem_ctl_link 代表的内存块链表
    struct mem_ctl_blk *sibling_next;
    char data[0];
} mem_ctl_blk;
```

由相同大小的内存块构成的内存块链表结构：

```c
// 组织一个链表，链表内的所有内存块的空间大小为一个范围
typedef struct mem_ctl_link {
    // head->next == tail 代表 mem_ctl_link 为空，此时需要从红黑树移除这个 link
    mem_ctl_blk head, tail;
    // block 的大小
    unsigned int blk_size;
} mem_ctl_link;

```



malloc 设计思路：
调用 malloc(size) 内部：去内部维护的一个红黑树上去寻找一个 key 最接近 size+sizeof(mem_ctl_blk) 的节点。如果找到了该节点，则从该节点代表的的内存块链表中取出一块内存，如果取出一块内存后，该链表长度变为0，则将该节点从红黑树移除。对取出的内存块进行分割，一部分分配出去，给malloc的调用者使用，另一部分放回红黑树，等待下次使用。

malloc 伪代码：

```c
void* malloc(size_t size) {
    // 获得内存管理链表
    mem_ctl_link *link = rb_search_most_close(size);
    if (link==NULL) {
        // 创建一个 blk，再 return
    }
    // 从链表中获取，并移除内存块
    mem_ctl_blk *blk = mem_ctl_link_pop_blk(link);
    if (is_link_empty(link)) { // 如果链表内部没有内存块了，那么从红黑树把该链表移除
        rb_erase(real_size);
    }
    // 如果剩余的块，可以构造一个内存块
    if (mem_ctl_blk_get_data_size(blk) > size + sizeof(mem_ctl_blk)) {
        // 多余空间对应的块
        mem_ctl_blk *ret_blk = (mem_ctl_blk*)(blk->data + size);
        // 更新两个的内存块的链接关系
        if ((ret_blk->addr_next = blk->addr_next)) {
            blk->addr_next->addr_last = ret_blk;
        }
        ret_blk->addr_last = blk;
        blk->addr_next = ret_blk;
        
		mem_ctl_link *new_link = rb_search(mem_ctl_blk_get_data_size(ret_blk));
        if (new_link==NULL) {
            new_link = mem_ctl_link_new(new_size);
            rb_insert(new_link);
        }
        mem_ctl_link_push(new_link, blk);
    }
    return blk->data;
}
```



free 设计思路：

调用 free(addr) 内部：通过对addr进行地址偏移，获得内存块控制单元地址，通过控制单元内部的控制信息，进行内存回收、空闲空间的合并

伪代码：

```c
void free(void* addr) {
    // 通过地址偏移，获得控制单元
    mem_ctl_blk *blk = ((char*)addr) - sizeof(mem_ctl_blk);
    if (mem_ctl_blk_is_free(blk->addr_next)) {
        // mem_ctl_blk_merge 内部会完成和 mem_ctl_blk_link、retree 的交互
        blk = mem_ctl_blk_merge(blk, blk->addr_next);
    }
    if (mem_ctl_blk_is_free(blk->addr_last)) {
        blk = merge(blk->addr_last, blk);
    }
    // 通过 blk 和 blk->addr_last 可以推测出内存块的大小
    const unsigned int blk_size = mem_ctl_blk_get_size(blk);
    
    // 把旧的内存块放回红黑树
    mem_ctl_link *new_link = rb_search(blk_size);
    if (new_link==NULL) {
        new_link = mem_ctl_link_new(blk_size);
		rb_insert(new_link);
    }
    mem_ctl_link_push(new_link, blk);
    rb_insert(new_link);
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
    mem_ctl_blk *blk = ((char*)addr) - sizeof(mem_ctl_blk));
    if (mem_ctl_blk_is_free(blk->addr_next) && mem_ctl_blk_get_size(blk->addr_next)+mem_ctl_blk_get_size(addr) >= size) {
        // 内部完成链表的断开工作
        mem_ctl_blk_get_space_h(blk->addr_next);
        // 将blk->addr_next 放回红黑树
        rb_insert_blk(blk->addr_next);
        return blk;
    }
    void *new_space = malloc(size);
    mem_move(new_space, addr, mem_ctl_blk_get_size(blk)-sizeof(mem_ctl_blk));
    free(addr);
    return (new_space);
}
```



设计完伪码后，考虑到有很多代码重复，后面会将重复代码合成函数

考虑对设计进行一次分层，初步分为 基础数据结构层、中间层、核心函数层：

1. 基础数据结构层：实现和管理 红黑树 和 mem_ctl_link、外界可以对 mem_ctl_link 进行操作，并对红黑树进行基本操作
2. 中间层：与 红黑树 和 mem_ctl_link 进行交互，对外隐藏 mem_ctl_link，对核心函数层提供支持
3. 核心函数层：依赖 中间层，实现 malloc、calloc、realloc、free 这些核心函数



红黑树和 mem_ctl_blk 的内存分配，要分离开，便于管理

底层的系统调用，预计使用 mmap 和 munmap。

每个线程都维护一个红黑树，考虑这些红黑树都从一个地方获取空间

