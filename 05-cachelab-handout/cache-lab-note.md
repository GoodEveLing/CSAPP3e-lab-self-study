# 任务需求
- partA: 实现一个cache模拟器
- partB: 优化一个矩阵转置函数,目标是最小化cache misses数量

# partA: 实现一个cache模拟器
> 熟读6.3章节

## 如何测试？

- 测试单个文件
```unix
unix> ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>

example:
unix> ./csim -s 1 -E 1 -b 1 -t traces/yi2.trace

```
- 测试csim-ref和自己的csim
```unix
make && ./test-csim
```

## 前置知识
- cache结构
  s：cache set数量=2^s;
  b：单个cache line中data block size=2^b;
  E：cache set内cache line数量=2^E;
- 如何解析address以找到缓存块?
  - address = tag + index + offset = m bits，addr结构如下：
  |tag|set index|block offet|
  |----|----|----|
  |t bits|s bits|b bits|

- 缓存命中与不命中？
  - 在缓存中查找数据时，如果找到，则命中，否则为不命中。
- cache miss的种类和原因?
  - 冷缓存：在cache刚初始化时，cache都是无效的，所以无法命中。不命中后需要执行相应的缓存放置策略
  - 容量不命中：cache容量太小导致不命中
- cache miss怎么处理?
  - 驱逐算法
  

## 核心实现
### 解析命令行输入参数
> 学习getopt函数的使用（和shell脚本中用法很相似）

### init_cache
> 二维数组的初始化

### 解析trace file
> 文件打开和读取的使用
> 按行解析trace file，将trace file中的地址解析成tag,index,block offset

### find_cache
> cache miss和hit的判断
> cache miss的处理
> LRU算法原理

# partB: 优化一个矩阵转置函数
> 熟读6.4章节
> 安装valgrind：`sudo apt-get install valgrind`

## 性能评估
- 32 × 32: 8 points if m < 300, 0 points if m > 600
- 64 × 64: 8 points if m < 1, 300, 0 points if m > 2, 000
- 61 × 67: 10 points if m < 2, 000, 0 points if m > 3, 000

## 测试方法
```unix
linux> make
linux> ./test-trans -M 32 -N 32
```

可以用csim-ref来测试具体的cache misses情况
```unix
./csim-ref -v -s 5 -E 1 -b 5 -t trace.f0 > hitf
```
在hitf文件中查看cache访问的情况

## 使用cache的结构
s = 5， E = 1，b = 5. 所以cache set num = 32 = cache line num，一个cache line只有一个data block,data block size = 32 bytes，total cache data size = 32 * 32 = 1024 bytes

矩阵一个元素占4字节，一个cache line可以存储8个矩阵元素，总共可以存 8 *32 = 256 个元素

## 32x32优化思路
测试trans.c中已给的一个example函数trans(),结果是
`hits:869 misses:1184 evictions:1152`

### 分析原因

- AB矩阵的地址？

- AB矩阵与cache映射关系？

A矩阵中的元素与cache的映射关系：
|A[0][0]~A[0][7]|A[0][8]~A[0][15]|A[0][16]~A[0][23]|A[0][24]~A[0][31]|
|----|----|----|----|
|line0|line1|line2|line3|

B矩阵同理。

A[i][j] = A[i * 32 + j]
B[j][i] = B[j * 32 + i]
- cache miss理论计算？
  - A矩阵按行遍历，遍历第i行时，A[i][0]~A[i][7] 会查看第k个cache line,cache miss后会将A矩阵的元素替换到cache line中。所以每行有32/8 = 4个cache miss, 其余hit，总cache miss数量 = 32 * 4 = 128

  - B矩阵按列遍历，遍历第i列时， 每列一开始都会发生cache miss,然后将B矩阵按行取8个元素写入cache line,但是遍历到B[j+1][i]时后面的元素并没有写入缓存，所以还是会cache miss。所以每列有32 cache miss，总cache miss数量 = 32 * 32 = 1024

  total cache miss数量 = 128 + 1024 = 1152

- 为什么实际cache miss数量比理论值要大？
  - 当i = j时，A[i][j]和B[i][j]会使用同一个cache line，此时会发生cache miss，这叫做冲突不命中，抖动`thrash`--高速缓存反复在A和B的块之间抖动。
  - 
### 分块优化
- 重点是减少B矩阵cache miss数量，尽可能使用cache line的B元素
- 使用局部变量存储A矩阵的元素，减少cache抖动
- 按照hints中提到的使用分块技术
  - 分块大小多少比较合适？
    按照cache line中data block恰好能放8个int，所以分块大小为8 * 8

## 64x64优化思路

## 61x67优化思路
# 遇到的问题
- index\tag数据类型写太小，导致寻找cache set错误
- 解析trace file 格式错误，导致解析结果很奇怪
- 矩阵转置优化好难(哭)

