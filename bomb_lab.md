# CSAPP3e-lab-self-study

# 前置知识

## 反汇编--objdump

```bash
objdump -S bomb
```

## x86常用寄存器
- eax: 通常用来执行加法，**函数调用的返回值**一般也放在这里面
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
栈是从高地址向低地址生长的。

输入三个参数，从右到左依次为：num3, num2, num1依次入栈，所以num1在栈顶，num2在栈顶+4，num3在栈顶+8。
```c
func(num1, num2, num3){}
```

## x86_64函数输入参数
- 当参数少于7个时， 参数从左到右放入寄存器:** rdi, rsi, rdx, rcx, r8, r9**
- 当参数为7个以上时， 前 6 个与前面一样， 但后面的依次从 “右向左” **放入栈中**，即和32位汇编一样。

## 字符串格式化读取函数sscanf函数传参
```c
#include <stdio.h>
int sscanf(const char *str, const char *format, ...);
```
在调用 sscanf 的过程中，参数会以以下方式传递：
- 第一个参数（输入字符串）str放在 %rdi。
- 第二个参数（格式字符串）format放在 %rsi。
- 第三个到第六个参数（六个数字的地址）表示解析结果，依次放在 %rdx, %rcx, %r8, %r9。
- 如果 sscanf 需要更多的参数，会继续使用栈。

## 常用指令
```armasm
mov %eax, %ecx ; ecx = eax
sub $1, %edx ; edx --
add $1, $edx ; edx ++
jm 0x12345678 ; jump to 0x12345678 addr
cmp $edx, $ecx ;比较ecx和edx的大小，通常和jmp跳转指令一起使用
callq 0x1234 <func1> ; 调用函数func1
lea    0x4(%rsp),%rbx ；rbx = 0x4 + rsp
mov    -0x4(%rbx),%eax ； eax = *(rbx - 0x4)
movzbl (%rbx,%rax,1),%ecx ；ecx = *(rbx + rax * 1) 
```

### lea和mov指令的区别
- mov 是用于将内存中的内容移动到寄存器中。
- lea 是用于计算一个地址并将这个地址存入寄存器。

```armasm
mov 0x4(%rsi), %rcx; rcx = *(rsi + 0x4)
lea 0x4(%rsi), %rcx; rcx = rsi + 0x4
```

## GDB commands
https://blog.csdn.net/weixin_42074738/article/details/134361545

- run gdb
```
gdb bomb
```
- examine命令 x ：查看内存地址中的值
  格式：x/<n/f/u>  <addr>
- print命令 p ：打印指针指向的内容
  格式：p *d@n
  d-地址
  n-打印的数量

## 如何拆炸弹？
bomb.c中提到了使用方法：
  ```c
	printf("Usage: %s [<input_file>]\n", argv[0]);
  ```

```bash
./bomb all_answers.txt
```

也可以在提示时手动敲答案到交互终端

# phase_1

## 分析main函数到phase_1()调用过程

```armasm
  400e32:	e8 67 06 00 00       	call   40149e <read_line>
  400e37:	48 89 c7             	mov    %rax,%rdi
  400e3a:	e8 a1 00 00 00       	call   400ee0 <phase_1>
  400e3f:	e8 80 07 00 00       	call   4015c4 <phase_defused>
```

```c

main(){
    ......
    rax = read_line();
    rdi = rax;//获取read_line返回值，即我们的输入字符串
    phase_1(rdi); //测试phase_1函数是关键
    phase_defused();
    ......
}
```

```armasm
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	call   401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	call   40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	ret   
```

```c
phase_1(rdi){
    rsp -= 8
    esi = 0x402400 //这个应该是要比较的字符串地址
    //从函数名可以看出要比较输入字符串rdi和rsi处字符串是否相等
    eax = strings_not_equal(rdi,rsi)

    if(eax == 0){
       rsp += 8;
       return;
    }
    //如果两个字符串不相等，就爆炸了
    explode_bomb()

    rsp += 8;
    return;

}
```

分析到phase_1解决关键了，我们先读下0x402400是啥字符串
```bash
(gdb) x/s 0x402400
0x402400:       "Border relations with Canada have never been better."
```
所以我们大胆猜测一下，如果输入的字符串和0x402400相等，就解决phase_1了。可以看到已经成功了！

```bash
(gdb) run
Starting program: xxx/bomb/bomb 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
```

再看看是怎么爆炸的。

## 炸弹怎么爆炸的？

```asmarm
000000000040143a <explode_bomb>:
  40143a:	48 83 ec 08          	sub    $0x8,%rsp
  40143e:	bf a3 25 40 00       	mov    $0x4025a3,%edi
  401443:	e8 c8 f6 ff ff       	call   400b10 <puts@plt>
  401448:	bf ac 25 40 00       	mov    $0x4025ac,%edi
  40144d:	e8 be f6 ff ff       	call   400b10 <puts@plt>
  401452:	bf 08 00 00 00       	mov    $0x8,%edi
  401457:	e8 c4 f7 ff ff       	call   400c20 <exit@plt>
```

```c
explode_bomb(){
    rsp -= 8
    edi = 0x4025a3
    puts(edi) //打印edi地址处的字符串
    edi = 0x4025ac
    puts(edi)
    edi = 8
    exit@plt
}
```
gdb 调试看看0x4025a3和0x4025ac是打印的啥？

```bash
(gdb) x/s 0x4025a3
0x4025a3:       "\nBOOM!!!"
(gdb) x/s 0x4025ac
0x4025ac:       "The bomb has blown up."
```
## 详解strings_not_equal()

再回到phase_1里面看看是怎么比较字符串的。

```armasm

0000000000401338 <strings_not_equal>:
  401338:	41 54                	push   %r12
  40133a:	55                   	push   %rbp
  40133b:	53                   	push   %rbx
  40133c:	48 89 fb             	mov    %rdi,%rbx
  40133f:	48 89 f5             	mov    %rsi,%rbp
  401342:	e8 d4 ff ff ff       	call   40131b <string_length>
  401347:	41 89 c4             	mov    %eax,%r12d
  40134a:	48 89 ef             	mov    %rbp,%rdi
  40134d:	e8 c9 ff ff ff       	call   40131b <string_length>
  401352:	ba 01 00 00 00       	mov    $0x1,%edx
  401357:	41 39 c4             	cmp    %eax,%r12d
  40135a:	75 3f                	jne    40139b <strings_not_equal+0x63>
  40135c:	0f b6 03             	movzbl (%rbx),%eax
  40135f:	84 c0                	test   %al,%al
  401361:	74 25                	je     401388 <strings_not_equal+0x50>
  401363:	3a 45 00             	cmp    0x0(%rbp),%al
  401366:	74 0a                	je     401372 <strings_not_equal+0x3a>
  401368:	eb 25                	jmp    40138f <strings_not_equal+0x57>
  40136a:	3a 45 00             	cmp    0x0(%rbp),%al
  40136d:	0f 1f 00             	nopl   (%rax)
  401370:	75 24                	jne    401396 <strings_not_equal+0x5e>
  401372:	48 83 c3 01          	add    $0x1,%rbx
  401376:	48 83 c5 01          	add    $0x1,%rbp
  40137a:	0f b6 03             	movzbl (%rbx),%eax
  40137d:	84 c0                	test   %al,%al
  40137f:	75 e9                	jne    40136a <strings_not_equal+0x32>
  401381:	ba 00 00 00 00       	mov    $0x0,%edx
  401386:	eb 13                	jmp    40139b <strings_not_equal+0x63>
  401388:	ba 00 00 00 00       	mov    $0x0,%edx
  40138d:	eb 0c                	jmp    40139b <strings_not_equal+0x63>
  40138f:	ba 01 00 00 00       	mov    $0x1,%edx
  401394:	eb 05                	jmp    40139b <strings_not_equal+0x63>
  401396:	ba 01 00 00 00       	mov    $0x1,%edx
  40139b:	89 d0                	mov    %edx,%eax
  40139d:	5b                   	pop    %rbx
  40139e:	5d                   	pop    %rbp
  40139f:	41 5c                	pop    %r12
  4013a1:	c3                   	ret    
```

```c
// 比较两个字符串是否相等, 返回0表示相等，返回1表示不相等
strings_not_equal(rdi, rsi){
    rbx = rdi //first argument
    rbp = rsi //second argument
    eax = string_length(rdi);//计算第一个字符串长度
    r12d = eax
    rdi = rbp
    eax = string_length(rdi);//计算第二个字符串长度
    edx = 1
    //如果len2 != len1
    if(r12d != eax){
        eax = edx = 1
        return eax;
    }
    //如果第一个字符串为空
    eax = (uint8_t)*rbx  //al = (uint8_t)eax
    if(eax == `\0`){
        edx = 0
        eax = edx = 0
        return eax
    }

    do{
        //按字节比较两个字符串
        if(rbp == al){
            //移动指针
            rbx ++;
            rbp ++;
            eax = (uint8_t)*rbx;
        }
        
        if(rbp != al){
            edx = 1;
            eax = edx = 1;
            return eax;
        }

    }while(eax != \0)

    edx = 0
    eax = edx = 0
    return eax

}

string_length(rdi){
    //字符串为空串，返回0
    if(rdi == 0){
        eax = 0
        return 0;
    }

    for(rdx = rdi; rdx != 0;  rdx ++;){
        eax = edx;
        eax -= edi //edi是字符串起始地址，eax是指针移动到的地址，相减就是字符串长度
    }

    return eax;
}
```

# phase_2

## 分析phase_2汇编代码
```armasm
00000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	call   40145c <read_six_numbers>
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	call   40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>

  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	call   40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx
  400f29:	48 39 eb             	cmp    %rbp,%rbx
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>

  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>

  400f3c:	48 83 c4 28          	add    $0x28,%rsp
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	ret    
```

可以发现400f2c -> 400f17, 400f3a -> 400f17`可能`是一个循环，我们详细看看。

```c

phase_2(rdi){

    rsi = rsp

    read_six_numbers(rdi, rsi)

    if(*rsp != 1)
        explode_bomb()
    // 也就是说第一个数字必须是1

    rbx = 4 + rsp // 2nd number
    rbp = rsp + 0x18// 边界
    do {
        eax = *(rbx - 0x4) // eax是rbx的上一个数字
        eax += eax

        if(eax != *rbx) //下一个数字是上一个数字的两倍
            explode_bomb()
        
        rbx += 4； //循环变量，也就是说rbx是第2~6个数字
    }while(rbp != rbx)

    return；
}
```
解题关键就是输入6个数字，然后要遵循如下规则：
- 第一个数字必须是1
- 下一个数字是上一个数字的两倍
答案显而易见：1 2 4 8 16 32

## 分析read_six_numbers()

```armasm
000000000040145c <read_six_numbers>:
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	call   40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	ret    

```

```c
read_six_numbers(rdi, rsi){
    rsp -= 0x18 // 24bytes, 存放6个4bytes数

    rdx = rsi //rsi为phase_2的栈顶指针， first number
    rcx = rsi + 4 // 2nd number

    rax = rsi +0x14 // 20 btyes, sixth number
    *(rsp + 8) = rax //存sixth number
    
    rax = rsi + 0x10 //fifth number
    *rsp = rax //存第5个数

    r9 = rsi + 0xc //fourth number
    r8 = rsi + 8 //third number

    /* 可以看出，6个数字的将依次放在rsi + (n - 1） * 4的位置，在phase_2函数中可以用rsp + (n - 1) * 4来获取第n个数字 */

    esi = 0x4025c3 //输入格式
    eax = 0
    eax = sscanf()//输入6个数字

    // 输入数字个数大于5就返回，否则输入有误炸弹爆炸
    if(eax > 5){
        rsp += 0x18
        return eax;
    }

    explode_bomb()

}

```

通过gdb查看输入留个数字的格式是什么

```bash
(gdb) x/s 0x4025c3
0x4025c3:       "%d %d %d %d %d %d"
```

# phase_3

## 输入字符串格式

```armasm
00400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	call   40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a> # 这个地址就是explode_bomb
```

```c
phase_3(char *input){
    rcx = rsp + 0xc // 第2个参数
    rdx = rsp + 0x8 // 第1个参数

    //通常 %esi 会作为 sscanf 函数的格式字符串参数（即格式控制符）
    esi = 0x4025cf //gdb查看format格式字符串
    eax = 0
    //由于 sscanf 返回匹配的项数，%eax 通常用于返回值（这里的 0 表示 sscanf 函数尚未执行，或者此时没有实际返回结果）。
    eax = sscanf(input, "%d %d", rdx, rcx)

    //输入参数数量少于2个，则报错
    if(eax <= 1){
        explode_bomb()
    }
    // 如果第一个参数 > 7，则报错
    if(*(rsp + 8) > 7){
        explode_bomb()
    }
```

现在知道这个实验要输入两个整数，第一个数字要小于等于7

## 分析输入数字的更多条件
```armasm
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax 
  400f75:	ff 24 c5 70 24 40 00 	jmp    *0x402470(,%rax,8)
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
```
eax = *(0x8 + rsp),也就是说eax等于第一个输入数字的值。在一句特殊的汇编`jmp    *0x402470(,%rax,8)`之后开始重复一段相似的操作,重复了8次：对eax寄存器赋值，然后跳转到0x400fbe的位置。
```asm
  400f7c:	b8 cf 00 00 00       	mov    $0xXXX,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
```
我们再看看0x400fbe是什么操作：
```asm
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	call   40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	ret    
```
```c
    // eax的值必须和第二个数字一样，否则爆炸
    if(eax != *(rsp + 0xc))
        explode_bomb()

    rsp += 0x18
    return;
```
到这里可以分析出，输入两个数字。第一个输入参数eax经过一段神奇操作后被赋值某些值，然后和第二个数字比较，如果相等就返回，否则就爆炸。大胆猜测那段操作类似switch-case语句。

## jmp    *0x402470(,%rax,8)

这句表示跳转到`*0x402470 + 8 * rax`的位置.即`(8*rax + 0x400f7c)`，根据rax的值跳转到不同位置，0x400f7c恰好是第一个重复操作地址，写出C伪代码。

gdb查看0x402470的值：
```bash
(gdb) x 0x402470
0x402470:       0x00400f7c
```
## swtich-case

```c
    eax = 8 + rsp = rdx
    //相对跳转
    jump to (8*rax + *0x402470) ==(8*rax + 0x400f7c)

    switch(eax){
        case 0 :
            eax = 0xcf
            break;
        case 1：
            eax = 0x2c3
            break;
        case 2：
            eax = 0x100
            break;
        case 3：
            eax = 0x185
            break;
        case 4：
            eax = 0xce
            break;
        case 5：
            eax = 0x2aa
            break;
        case 6:
            eax = 0x147
            break;
        case 7:
            explode_bomb()
    }

```
到这里我们可以写出所有答案：
| 输入1 | 输入2       |
| ------- | ------------- |
| 0     | 0xcf = 207  |
| 1     | 0x2c3 = 707 |
| 2     | 0x100 = 256 |
| 3     | 0x185 = 389 |
| 4     | 0xce = 206  |
| 5     | 0x2aa = 682 |
| 6     | 0x147 = 327 |

# phase_4

汇编前面部分和phase_3一样，这里不再赘述：
- **输入两个整数**
- 第一个数字要小于等于14
- 
```c
phase_4(){

    rcx = rsp + 0xc // 2nd input
    rdx = rsp + 8// first input

    esi = 0x4025cf
    eax = 0
    sscanf("%d %d", &rcx, &rdx)

    if(eax != 2)
        explode_bomb()

    //第一个数字要小于等于14
    if(0xe < *(rsp + 8))    
        explode_bomb()
```

继续分析

```armasm
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx
  40103f:	be 00 00 00 00       	mov    $0x0,%esi
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  401048:	e8 81 ff ff ff       	call   400fce <func4>
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	call   40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	ret    
```

```c
    edx = 0xe
    esi = 0
    edi = *(8 + rsp) // edi就是第一个输入参数

    eax = func4(edi, esi, edx)
    //返回值必须为0
    if(eax != 0)
        explode_bomb()

    //第2个参数必须为0
    if(*(0xc + rsp) ！= 0)
        explode_bomb()

    return 
}
```
虽然不知道func4干了啥，但是得到一条重要信息：**第2个参数必须为0**

## 分析func4

```armasm
0000000000400fce <func4>:
  400fce:	48 83 ec 08          	sub    $0x8,%rsp
  400fd2:	89 d0                	mov    %edx,%eax
  400fd4:	29 f0                	sub    %esi,%eax
  400fd6:	89 c1                	mov    %eax,%ecx
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx
  400fdb:	01 c8                	add    %ecx,%eax
  400fdd:	d1 f8                	sar    %eax
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx
  400fe2:	39 f9                	cmp    %edi,%ecx
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>
  400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
  400fe9:	e8 e0 ff ff ff       	call   400fce <func4>
  400fee:	01 c0                	add    %eax,%eax
  400ff0:	eb 15                	jmp    401007 <func4+0x39>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax
  400ff7:	39 f9                	cmp    %edi,%ecx
  400ff9:	7d 0c                	jge    401007 <func4+0x39>
  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	call   400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	ret    

```
在400fe9和400ffe都调用了func4，可以得知是**递归函数**；

400ff2和400ff9都跳转到401007，这里显然是return，递归出口；

400fe4跳转到400ff2是个if判断

```c
func4(edi, esi, edx){
    eax = edx          //eax = 14
    eax = eax - esi    // eax = eax
    ecx = eax          //输入参数的差值 ecx = edx - esi = 14
    ecx >>= 0x1f       // 0xe >> 31 = 0, 得到符号位
    eax += ecx         // eax = 14 + 0 = 14
    eax >>= 1          //eax / 2，符号位不变，eax = 7

    /* eax = eax - esi， eax = 14
       eax = (eax + eax >> 31)/2
    */

   // ecx = eax + esi = (eax + eax >> 31)/2, 计算得到ecx = 7
    ecx = rax + rsi * 1 // rcx = rax == 7

    if(ecx <= edi){
        eax = 0
        // 第1个参数 <= 7退出，此时eax = 0,退出. 破解成功
        // 结合外面的if判断，7<= edi <= 7，所以edi = 7时，return 0
        // 也就是说edi == ecx即可正确退出！！！
        if(ecx >= edi)
            return eax

        esi = rcx + 1
        func4(edi,esi, edx);
        eax = 0x1 + (%rax + %rax * 1) = 1 + 2*eax
    }else{
        edx = ecx - 1
        func4(edi,esi, edx);
        eax *= 2
    }
    return eax
}

```
此时，得到输入参数1的答案之一：7

另外，edi == ecx即可正确退出！！！

```c
func4(edi, esi, edx){
    eax = edx          
    eax = eax - esi    
    ecx = eax         
    ecx >>= 0x1f     
    eax += ecx     
    eax >>= 1    
    ecx = rax + rsi * 1

    // ecx = [bit32(edx - esi) + (edx - esi)] >> 1 + rsi

    if(ecx <= edi){
        if(ecx >= edi)
            return 0

        esi = rcx + 1
        return 1 + 2 * func4(edi, esi, edx);
    }else{
        edx = ecx - 1
        return 2 * func4(edi,esi, edx);
    }
}
```

- 假如edi < 7, 需要递归返回 0才行，
```c
0 = func4(edi, 0, 6)

ecx = 6 >> 1 = 3
```
- 假如edi < 3,
```c
0 = func4(edi, 0, 2)

ecx = 2 >> 1 = 1
```
- 假如edi < 1,
```c
0 = func4(edi, 0, 1)

ecx = 1 >> 1 = 0
```

至此，我们得到所有答案：
| 输入1 | 输入2 |
| ------- | ------- |
| 7     | 0     |
| 3     | 0     |
| 1     | 0     |
| 0     | 0     |

# phase_5
## 输入字符串

前面输入字符串逻辑和前面类似，这里不再赘述：
- 需要输入6个字符的字符串

```c
phase_5(rdi){
    rbx = rdi  //输入字符串
    rax = *(fs + 0x28)  
    *(rsp + 0x18) = rax 

    eax = 0
    //输入字符串长度 == 6
    eax = string_length()
    if(eax != 6)
        explode_bomb()
```
## 循环 & char array

```armasm
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
```
解析完字符串后，跳转到4010d2，紧接跳转到40108b

40108b->4010ac类似一个循环：

```armasm
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```

```c
eax = 0
do{
    ecx = *(ebx + rax * 1)    
    *rsp = cl = (uint8_t)ecx     // cl表示rcx low 8 bits
    rdx = *rsp  
    //rdx = rdi + *(fs + 0x28)
    edx = edx & 0xf  // get 低4位的值

    // “maduiersnfotvbylSo”
    edx = *(0x4024b0 + rdx) // key point!!!!
    *(0x10 + (rsp + rax * 1)) = dl//rdx low 8 bits

    rax ++
}while(rax != 6)
```

再优化一下：
```c
for(rax = 0; rax != 6; rax ++){
    ecx = *(ebx + rax * 1) = *(edi + rax) //ecx是输入字符串的第rax个字符，按1B移动指针

    *rsp = cl = (uint8_t)ecx     // cl表示rcx low 8 bits
    rdx = *rsp     //字符的8bit值
    //rdx = rdi + *(fs + 0x28)
    edx = edx & 0xf  // get 低4位的值

    // “maduiersnfotvbylSo”
    edx = *(0x4024b0 + rdx * 1) // 在0x4024b0地址上偏移字符的assic大小取值
    *(0x10 + (rsp + rax * 1)) = dl//rdx low 8 bits，然后往栈上rsp + 0x10开始放rdx的低8位数据
}
```
看看0x4024b0放的什么东西，是一个数组？？
```bash
(gdb) x 0x4024b0
0x4024b0 <array.3449>:  0x7564616d
```

继续看不同格式下是什么，是一个char array

```bash
(gdb) x/s 0x4024b0
0x4024b0 <array.3449>:  "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```
**至此我们输入的字符assic低8位值用来作为在char array的索引，取出的字符会放在rsp + 0x10开始的位置，会放6个字符。**

继续分析输入的字符需要满足什么条件？

## 寻找更多输入条件

```armasm
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  4010bd:	e8 76 02 00 00       	call   401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>
  4010c6:	e8 6f 03 00 00       	call   40143a <explode_bomb>
  4010cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  4010d0:	eb 07                	jmp    4010d9 <phase_5+0x77>
```

```c
    *(0x16 + rsp) = 0x0
    esi = 0x40245e
    rdi = *(0x10 + rsp)  //取出刚刚从char array中取到的字符串
    eax = strings_not_equal(rdi, esi)
    if(eax != 0){   
        explode_bomb()
    }else{
        break;
    }        
```
比较0x40245e地址处的东西和从char array中取到的字符串，得到关键信息：这俩字符串必须一样，否则爆炸！

看看0x40245e地址处是什么？这里我们可以猜到这里是一个字符串。
```bash
(gdb) x/s 0x40245e
0x40245e:       "flyers"
```

到这里，我们得到**输入的所有条件**：
- 输入一个字符串，包含6个字符
- 根据输入字符的assic低4位做索引值，从0x4024b0地址上取字符，取6个字符
- 然后和0x40245e地址处的字符串比较，必须一样

 f = + 9
 l = +15 = f
 y = +14 = e
 e = +5
 r = +6
 s = +7

答案不固定，下面是一些举例：
- 9ONEFG
- 9/.%&'
- 9?>567
- IONEFG

# phase_6
## 输入格式
```armasm
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  401100:	49 89 e5             	mov    %rsp,%r13
  401103:	48 89 e6             	mov    %rsp,%rsi
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
  40110b:	49 89 e6             	mov    %rsp,%r14
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d
```

```c
    rsp -= 0x50
    r13 = rsp 
    rsi = rsp

    read_six_number()

    r14 = rsp 
    r12d = 0
```

## 检查输入数字条件1

```armasm
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
```

```c
while(1){
    rbp = r13  //r13 就是指向6个数字的指针，依次移动4字节
    eax = *(r13) //获取输入的数字
    eax --
    if(eax > 0x5) 
      explode_bomb()
    // number - 1 > 5就爆炸，所以输入参数 <= 6

    r12d ++;
    if(r12d == 0x6){   
      //！！！！跳出循环的条件是 r12d == 0x6；也就是这个while循环六次
      goto 401153
    }
    
    // 检查每个元素之间是否相等，*rbp = *(r13), r13从rsp按4字节移动
    for (ebx = r12d; ebx <= 5 ; ebx ++){
      rax = ebx
      eax = *(rsp + rax * 4) 
      if(eax == *rbp)
        explode_bomb()
    }
    r13 += 0x4
}
```
## 修改输入数字

```armasm
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  401158:	4c 89 f0             	mov    %r14,%rax
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx

  401160:	89 ca                	mov    %ecx,%edx
  401162:	2b 10                	sub    (%rax),%edx
  401164:	89 10                	mov    %edx,(%rax)
  401166:	48 83 c0 04          	add    $0x4,%rax
  40116a:	48 39 f0             	cmp    %rsi,%rax
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>
```

```c
    rsi = 0x18 + rsp  // 存放输入数字的边界
    rax = r14 // rax = r14 = rsp，指向数字的指针, 循环变量，按4B移动
    ecx = 0x7
    //将原来6个数字修改为0x7 - sum(num1~i)

    for(rax = r14; rsi != rax; rax += 4){
        edx = ecx = 0x7
        edx -= *(rax)
        // *rax = *(rsp) = edx = 0x7 - numi
        *rax = edx
    }
```
上面这个循环将原来6个数字修改为0x7 - 原数字，我们称之new number

## 输入数字条件2 --嵌套循环、链表
```armasm
 40116f:	be 00 00 00 00       	mov    $0x0,%esi
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>

  ; ecx > 1，循环体
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
  40117a:	83 c0 01             	add    $0x1,%eax
  40117d:	39 c8                	cmp    %ecx,%eax
  40117f:	75 f5                	jne    401176 <phase_6+0x82>
  401181:	eb 05                	jmp    401188 <phase_6+0x94>

  ; ecx <= 1 循环体
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
```

```c
for(esi = 0; esi != 0x18; rsi += 4){
  ecx = *(rsp + rsi * 1) // 遍历new number
  if(ecx > 1){
    edx = 0x6032d0
    for(eax = 1; eax! = ecx; eax ++){
      rdx = *(rdx + 8) //地址跳转ecx次，从0x6032d0开始取值，再根据值跳转到下一个地址，依次循环
      //rdx = node -> next
    }
  }else{
    edx = 0x6032d0
  }
  //选择的是遍历输入数字后的栈地址，根据array的元素将node分别存在stack上
  *(0x20 + rsp + rsi * 2) = rdx // 将node地址存到rsp + 0x20的位置上

}
```

上面的函数可以分析得到：
- 如果new_number == 1, 直接再栈上存0x6032d0的值
- 如果new_number > 1, 遍历new_number，再在0x6032d0处间接跳转new_number次，得到某个值，然后将值存到rsp + 0x20 到 rsp + 0x20 + 0x30的位置上, 每个元素是8B = 64 bit. 这里我们将这个位置开始数组称为array

看看0x6032d0就究竟存的什么：

```asm
(gdb) x/24 0x6032d0
    ; 0x6032d0地址处应该是个数组，根据第一个数的大小，得到一个数组
    ; node是个结构体，里面有4个元素
0x6032d0 <node1>:       0x0000014c      0x00000001      0x006032e0      0x00000000
0x6032e0 <node2>:       0x000000a8      0x00000002      0x006032f0      0x00000000
0x6032f0 <node3>:       0x0000039c      0x00000003      0x00603300      0x00000000
0x603300 <node4>:       0x000002b3      0x00000004      0x00603310      0x00000000
0x603310 <node5>:       0x000001dd      0x00000005      0x00603320      0x00000000
0x603320 <node6>:       0x000001bb      0x00000006      0x00000000      0x00000000
(gdb) x/24w 0x6032f0
    ; 可以发现第3个元素的值就是下一个node的地址，node6后面没有别的节点了。第一个元素是node value，第2个元素是node index
0x6032d0 <node1>:       332     1       6304480 0
0x6032e0 <node2>:       168     2       6304496 0
0x6032f0 <node3>:       924     3       6304512 0
0x603300 <node4>:       691     4       6304528 0
0x603310 <node5>:       477     5       6304544 0
0x603320 <node6>:       443     6       0       0
```
我们可以写出node的结构体：

```c
struct node{
    uint32_t value;
    uint32_t index;
    uint32_t* next_node;
}
```

## 输入数字条件3

```armasm
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
```

```c
  rbx = *(0x20 + rsp) // 第1个node
  rax = 0x28 + rsp //第2个node的指针
  rsi = 0x50 + rsp // 存放的边界
  rcx = rbx

  do{
      rdx = *rax //node2
      *(0x8 + rcx) = rdx //*(0x8 + node1) = node2
      //让栈内存放的node重新建立新的指向关系
      rax += 8

      if(rax == rsi)
        break;

      rcx = rdx
  } while( rsi != rax)
```
## 链表降序

```armasm
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
```

```c
  *(0x8 + rdx) = 0

  for(ebp = 0x5; ebp != 0; ebp--){
      rax = *(0x8+rbx) //  rbx = *(0x20 + rsp) 是node.next的地址，rax获得node.next.value
      eax = *rax // eax = node->next.value
      if(*rbx < eax) // node.value < node.next.value，所以应该降序排列
          explode_bomb()

      rbx = *(0x8 + rbx)
  }

  rsp += 0x50
  return eax
```

根据上面的分析可知，要让node value降序排列，那么对应降序排列的序号：3 4 5 6 1 2。

但是这个序号是经过修改的，所以原来的顺序应该是：4 3 2 1 6 5
