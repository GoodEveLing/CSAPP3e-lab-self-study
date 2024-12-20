# CSAPP3e-lab-self-study

# 前置知识
## x86常用寄存器
- eax: 通常用来执行加法，函数调用的返回值一般也放在这里面
- ebx: 数据存取
- ecx: 通常用来作为计数器，比如for循环
- edx: 读写I/O端口时，edx用来存放端口号
- esp: 栈顶指针，指向栈的顶部
- ebp: 栈底指针，指向栈的底部，通常用ebp+偏移量的形式来定位函数存放在栈中的局部变量
- esi: 字符串操作时，用于存放数据源的地址
- edi: 字符串操作时，用于存放目的地址的，和esi两个经常搭配一起使用，执行字符串的复制等操作

不同位数的寄存器，举例：
RAX是64位寄存器，可以拆分。例如我们操作EAX，就是在对RAX的低32位进行操作。同样以此类推，AX表示RAX的低16位，AH表示RAX低16位中的高8位，AL表示RAX低16位中的低8位。
## 函数入栈顺序
x86 架构中的函数参数在入栈时通常是按照 `从右到左` 的顺序入栈的。

## x86_64函数输入参数
- 当参数少于7个时， 参数从左到右放入寄存器: rdi, rsi, rdx, rcx, r8, r9。
- 当参数为7个以上时， 前 6 个与前面一样， 但后面的依次从 “右向左” 放入栈中，即和32位汇编一样。

## 常用指令
```asm
mov %eax, %ecx ; ecx = eax
sub
add
jm
cmp
callq
lea
movzbl

```

# phase_6

```asm

00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx
# ------------------------------------------------------------------
    rsp -= 0x50
    r13 = rsp 
    rsi = rsp

    read_six_number()

    r14 = rsp 
    r12d = 0
# ------------------------------------------------------------------

  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  401100:	49 89 e5             	mov    %rsp,%r13
  401103:	48 89 e6             	mov    %rsp,%rsi
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
  40110b:	49 89 e6             	mov    %rsp,%r14
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d

# ------------------------------------------------------------------
while(1){
    rbp = r13
    eax = *(r13)
    eax --
    if(eax > 0x5) 
      explode_bomb()

    r12d ++;
    if(r12d == 0x6){   
      ;！！！！跳出循环的条件是 r12d == 0x6；也就是这个while循环六次
      goto 401153
    }
    
    # 检查每个元素之间是否相等，被计较数是*rbp = *(r13), r13从rsp按4字节移动
    for (ebx = r12d; ebx <= 5 ; ebx ++){
      rax = ebx
      eax = *(rsp + rax* 4)
      if(eax == *rbp)
        explode_bomb()
    }
    r13 += 0x4
}
# ------------------------------------------------------------------

  401114:	4c 89 ed             	mov    %r13,%rbp
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax
  40111b:	83 e8 01             	sub    $0x1,%eax
  40111e:	83 f8 05             	cmp    $0x5,%eax
  401121:	76 05                	jbe    401128 <phase_6+0x34>
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
  401128:	41 83 c4 01          	add    $0x1,%r12d
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d
  401130:	74 21                	je     401153 <phase_6+0x5f>

  401132:	44 89 e3             	mov    %r12d,%ebx # 循环变量
  401135:	48 63 c3             	movslq %ebx,%rax
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
  40113e:	75 05                	jne    401145 <phase_6+0x51>
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
  401145:	83 c3 01             	add    $0x1,%ebx
  401148:	83 fb 05             	cmp    $0x5,%ebx
  40114b:	7e e8                	jle    401135 <phase_6+0x41>

  40114d:	49 83 c5 04          	add    $0x4,%r13
  401151:	eb c1                	jmp    401114 <phase_6+0x20>
  
# --------------------------------------------------------------
rsi = 0x18 + rsp
rax = r14 ; 指向数字的指针, 循环变量，按4B移动
ecx = 0x7
; 将原来6个数字修改为0x7 - sum(num1~i)
do{
    edx = ecx = 0x7
    edx -= *(rax)
    ; *rax = *(rsp) = edx = 0x7 - numi
    *rax = edx
    rax += 4
}while(rsi != rax) ; 保证指针移动不超过6个数字

# --------------------------------------------------------------
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  401158:	4c 89 f0             	mov    %r14,%rax
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx

  401160:	89 ca                	mov    %ecx,%edx
  401162:	2b 10                	sub    (%rax),%edx
  401164:	89 10                	mov    %edx,(%rax)
  401166:	48 83 c0 04          	add    $0x4,%rax
  40116a:	48 39 f0             	cmp    %rsi,%rax
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>
# --------------------------------------------------------------

for(esi = 0; esi != 0x18; rsi += 4){
  ecx = *(rsp + rsi * 1)
  if(ecx > 1){
    edx = 0x6032d0
    for(eax = 1; eax! = ecx; eax ++){
      rdx = *(rdx + 8)
      ; rdx = node -> next
    }
  }else{
    edx = 0x6032d0
  }
  ; 选择的是遍历【0x7 - sum(numi)】后的node地址，根据array的元素将node分别存在stack上
  *(0x20 + rsp + rsi * 2) = rdx ; 将node地址存到rsp + 20的位置上
}

# --------------------------------------------------------------
  40116f:	be 00 00 00 00       	mov    $0x0,%esi
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>

  ; ecx > 1
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
  40117a:	83 c0 01             	add    $0x1,%eax
  40117d:	39 c8                	cmp    %ecx,%eax
  40117f:	75 f5                	jne    401176 <phase_6+0x82>
  401181:	eb 05                	jmp    401188 <phase_6+0x94>

  ; ecx <= 1
  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)
  40118d:	48 83 c6 04          	add    $0x4,%rsi
  401191:	48 83 fe 18          	cmp    $0x18,%rsi
  401195:	74 14                	je     4011ab <phase_6+0xb7> ; 跳出循环
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx
  40119a:	83 f9 01             	cmp    $0x1,%ecx
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>

  ; ecx > 1
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>
# --------------------------------------------------------------
  rbx = *(0x20 + rsp)
  rax = *(0x28 + rsp)
  rsi = *(0x50 + rsp)
  rcx = rbx

  do{
      rdx = *rax
      *(0x8 + rcx) = rdx
      rax += 8

      if(rax == rsi)
        break;

      rcx = rdx
  } while( rsi != rax)
# --------------------------------------------------------------
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi
  4011ba:	48 89 d9             	mov    %rbx,%rcx

  4011bd:	48 8b 10             	mov    (%rax),%rdx
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)
  4011c4:	48 83 c0 08          	add    $0x8,%rax
  4011c8:	48 39 f0             	cmp    %rsi,%rax
  4011cb:	74 05                	je     4011d2 <phase_6+0xde>
  4011cd:	48 89 d1             	mov    %rdx,%rcx
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>
# --------------------------------------------------------------
  *(0x8 + rdx) = 0

  for(ebp = 0x5; ebp != 0; ebp--){
      rax = *(0x8+rbx) ;   rbx = *(0x20 + rsp) 应该是node.next的地址，rax获得node.next.value
      eax = *rax
      if(*rbx < eax) ; node.value < node.next.value，所以应该降序排列
          explode_bomb()

      rbx = *(0x8 + rbx)
  }

  rsp += 0x50
  return eax

# --------------------------------------------------------------

  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)
  4011d9:	00 
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp

  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax
  4011e3:	8b 00                	mov    (%rax),%eax
  4011e5:	39 03                	cmp    %eax,(%rbx)
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
  4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
  4011f2:	83 ed 01             	sub    $0x1,%ebp
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
  4011f7:	48 83 c4 50          	add    $0x50,%rsp
# --------------------------------------------------------------

  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	retq   
```
```bash
    ; 0x6032d0地址处应该是个数组，根据第一个数的大小，得到一个数组
    ; node是个结构体，里面有4个元素
0x6032d0 <node1>:       0x0000014c      0x00000001      0x006032e0      0x00000000
0x6032e0 <node2>:       0x000000a8      0x00000002      0x006032f0      0x00000000
0x6032f0 <node3>:       0x0000039c      0x00000003      0x00603300      0x00000000
0x603300 <node4>:       0x000002b3      0x00000004      0x00603310      0x00000000
0x603310 <node5>:       0x000001dd      0x00000005      0x00603320      0x00000000
0x603320 <node6>:       0x000001bb      0x00000006      0x00000000      0x00000000
    ; 可以发现第3个元素的值就是下一个node的地址，node6后面没有别的节点了。第一个元素是node value
0x6032d0 <node1>:       332     1       6304480 0
0x6032e0 <node2>:       168     2       6304496 0
0x6032f0 <node3>:       924     3       6304512 0
0x603300 <node4>:       691     4       6304528 0
0x603310 <node5>:       477     5       6304544 0
0x603320 <node6>:       443     6       0       0
```
降序排列的序号：3 4 5 6 1 2
array的元素应该为是上述排列
但是这个array是经过修改的，所以原来的顺序应该是：4 3 2 1 6 5
