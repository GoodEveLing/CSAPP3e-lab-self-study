# 任务要求
实现mm.c中的函数，即实现一个动态内存分配器

mm.c中已经有了一份基础代码，通过下列命令运行：
```
make //编译出mdriver可执行文件
./mdriver -V
```
可选输入选项参数：
- -t <tracedir>: Look for the default trace files in directory tracedir instead of the default directory defined in config.h.
- -f <tracefile>: Use one particular tracefile for testing instead of the default set of trace files.
- -h: Print a summary of the command line arguments.
- -l: Run and measure libc malloc in addition to the student’s malloc package.
- -v: Verbose output. Print a performance breakdown for each tracefile in a compact table.
- -V: More verbose output. Prints additional diagnostic information as each trace file is processed. Useful during debugging for determining which trace file is causing your malloc package to fail.

> 官网提供的tar文件中没有trace files, 可以下载本目录trace中的
> 修改config.h中宏 `TRACDIR` 为自己实际的trace目录地址

# 准备工作
修改makefile,保留更多调试信息，方便debug
```makefile
CC = gcc -g
CFLAGS = -Wall -O0 -m32
```

# 需要考虑的问题

在9.9.6中提到一个分配器需要考虑的问题：
1. 空闲块组织：如何记录空闲块？
   1. 一个数据块 = header + payload + footer
   2. 书上有提到加上footer的理由：方便合并空闲块
2. 放置：如何选择一个合适的空闲块来放置一个新分配的块？
   1. 放置策略：first fit, next fit, best fit
3. 分割：在将一个新分配的块放置到某个空闲块之后，如何处理这个空闲块中的剩余部分？
   1. 剩余部分变成一个新的空闲块
4. 合并：如何处理一个刚刚释放的块？
   1. 为什么要合并？
        解决假碎片问题
   2. 合并空闲块的时机？
       - 立即合并：在每次一个块释放时就合并所有相邻的块
       - 推迟合并：等到某个稍晚的时候再合并空闲块
   3. 合并的方式？
       - 合并后面的块只需要修改块大小即可
       - 合并前面的块就需要修改前面的块大小，然后怎么得到前一个块的header位置呢？只能从头开始搜索，时间复杂度高。所以给一个数据块添加footer，footer和header的结构内容一样。
5. 如果空闲块size依然不满足请求怎么办？
   1. 合并相邻空闲块为一个更大的，扩大空闲块size
   2. 如果在上一步依然无法满足需求呢？
      1. 请求更多的heap内存，转化为一个大的空闲块

# [v1]隐式空闲链表 + first fit
描述一个数据块的必要属性：
- 块大小
- 状态位：空闲 或 已分配
- payload

在csapp中有一个约束条件：双字对齐，也就是数据块8B对齐
最小的块大小 = header（1 word）+ footer(1 word) = 2 word
> 我们在分配时size要求大于0，所以实际分配的块大小最小值为：
> header(1word) + footer(1word) +payload(1word) = 3word，这个称为有效载荷
> header高32bit存放size + 3bit(flag), footer低32bit存放数据


# [v2]implicit list + next fit
> 在v1的基础上，调整搜索空闲块的策略：记录上一次空闲块的指针，这样每次搜索空闲块时，从上一次空闲块开始搜索，这样能减少搜索时间

## 需要考虑的问题
- 上一次空闲块指针初始化
- 何时更新这个指针
- 如何更新这个指针

# [v3] explicit free list + LIFO + first fit
> 在v2的基础上，调整空闲块组织方式：将所有空闲块组成一个链表，一个空闲块的前继节点和后续节点通过header后面的32bit+32bit分别记录，这样只需要修改一个节点的pre_pointer和next_pointer即可修改整个链表。这个改进用于提高find_fit的性能。
> 另外空闲块搜索采用LIFO策略，新的空闲块插入到链表头部，这样每次搜索空闲块时，从链表头部开始搜索。

数据块的组织结构：
- 已分配块 = header + payload
- 空闲块 = header + pre_pointer + next_pointer + payload + footer

## 需要考虑的问题
- **空闲块链表的初始化**:
  在mm_init()中初始化空闲链表， 链表头指向空。
   ```c
   free_list = NULL;
   ```
- **空闲块插入的实现和时机**
  - 在coalesce()可能会产生新的空闲块，需要插入到空闲链表中。
  - mm_free()会释放一个块，释放后需要从空闲链表中插入这个块。不过这个函数会调用coalesce()，不用执行插入操作，但是需要更新这个块的pred和succ的情况
  ```c
   PUT_PRED(ptr, NULL);
   PUT_SUCC(ptr, NULL);
   ```
- **空闲块被分配后的处理**
  - 在place()中这个块被分配后，需要从空闲链表中移除，前后节点的指针需要更新。
  - 同样也是在coalesce()中，如果合并了前后两个块，则需要删除原来的块。

# [v4] segreated list
> 调整空闲块组织方式：将所有空闲块组成多个链表，按照size大小进行分类，将空闲块插入到合适的链表中。这样在find_fit()中，只需根据size确定要搜索的链表，这样能提高find_fit()的性能。

## 需要考虑的问题
- 多个链表如何划分？
  - 按照size大小进行划分
- 多个链表存储在哪里？
- 如何调整insert和delete函数？
