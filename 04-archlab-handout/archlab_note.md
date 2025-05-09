# 任务需求理解 
[readme](http://csapp.cs.cmu.edu/3e/README-archlab)
[write up](http://csapp.cs.cmu.edu/3e/archlab.pdf)

- 目的：理解处理器设计和软硬件之间的联系
- 任务：设计实现一个Y86-64流水线处理器，提升在ncopy.ys程序上的性能。目标是最小化CPE的时间周期数量
- 具体操作
   1. partA：写一些简单的Y86-64程序，熟悉Y86-64工具
   2. partB：通过一个新的指令扩展SEQ模拟器
   3. partC：[最核心部分]优化Y86-64 benchmark程序和处理器设计

# 准备工作
- 解压sim.tar
```
tar xvf sim.tar
```
- 编译Y86-64 tools
```
cd sim
make clean; make
```
遇到一些编译失败问题
1. sim/misc/yas.h文件在int lineno 前加上extern
2. makefile 编译器选项加上-fcommon
```cmake
CFLAGS=-Wall -O1 -g -fcommon
LCFLAGS=-O1 -fcommon
```
# partA
工作在sim/misc目录下
具体任务是啥？--写和模拟3个Y86-64程序，这三个程序定义为examples.c中三个C函数
如何测试？--通过YAS程序汇编自己写好的程序，然后通过指令集模拟器YIS跑这些程序

## sum.ys
```c
/* linked list element */
typedef struct ELE {
    long val;
    struct ELE *next;
} *list_ptr;

/* sum_list - Sum the elements of a linked list */
long sum_list(list_ptr ls)
{
    long val = 0;
    while (ls) {
	val += ls->val;
	ls = ls->next;
    }
    return val;
}
```
需要将sum_list函数用sum.ys实现：求链表中元素值之和。目前的关键问题：
1. ys文件用什么指令描述？在isa.c中可以看到提供的指令集，类似x86-64汇编语言。
2. 如何改写这个函数？
  - 汇编表示函数的方法，函数参数入参
  - 循环的汇编表示
  - 存储求和值和返回该值
  - 链表的汇编表示,怎么获取链表的值+怎么跳转链表？
3. 参考CSAPP3e-lab-self-study/04-archlab-handout/sim/y86-code/asum.ys 里面的结构

- 注意那三个movq的区别，会不会和数据类型有关？
  - 前缀的表示：立即数(i),寄存器（r）,内存（m）
- `addq (%rcx), %rax`报错，用另一个寄存器存放（%rdi）后在和rax相加就不报错来，为什么？
  - OPq只对寄存器操作，（%rcx）表示寄存器取值
- 为什么在loop前后都有判空的语句？

```unix
cd sim/misc
./yas sum.ys
./yis sum.yo
```
若%rax寄存器值为0xcba则测试通过

## rsum.ys
递归版本的sum_list
- 递归的汇编实现

## copy.ys
参考 asum.ys实现

# partB
拓展SEQ模拟器取支持iaddq,优化seq-full.hcl

```shell
ssim.c:20:10: fatal error: tk.h: 没有那个文件或目录
```
[环境配置](https://blog.csdn.net/altoer/article/details/105116126)
## build and test
```
unix> make VERSION=full
unix> ./ssim -t ../y86-code/asumi.yo
```