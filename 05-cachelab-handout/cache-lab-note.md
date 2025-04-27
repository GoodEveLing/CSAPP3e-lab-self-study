# 任务需求

- partA: 实现一个cache模拟器
- partB: 优化一个矩阵转置函数,目标是最小化cache misses数量

# partA: 实现一个cache模拟器
>
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
>
> 学习getopt函数的使用（和shell脚本中用法很相似）

### init_cache
>
> 二维数组的初始化

### 解析trace file
>
> 文件打开和读取的使用
> 按行解析trace file，将trace file中的地址解析成tag,index,block offset

### find_cache
>
> cache miss和hit的判断
> cache miss的处理
> LRU算法原理

# partB: 优化一个矩阵转置函数
>
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

矩阵一个元素占4字节，一个cache line可以存储8个int，总共可以存 8 * 32 = 256个 int元素。

## 32x32优化思路

测试trans.c中已给的一个example函数trans(),结果是
`hits:869 misses:1184 evictions:1152`

### AB矩阵的地址？

相邻矩阵元素相差0x4的是A矩阵，相差0x8的是B矩阵。
hitf文件开始：

```txt
S 10c080,1 miss 
L 18c0c0,8 miss 
L 18c0a4,4 miss 
L 18c0a0,4 hit 
L 10c0a0,4 miss eviction  ==>A
S 14c0a0,4 miss eviction ==>B
L 10c0a4,4 miss eviction ==>A
S 14c120,4 miss ==>B
L 10c0a8,4 hit 
S 14c1a0,4 miss 
L 10c0ac,4 hit 
S 14c220,4 miss 
L 10c0b0,4 hit 
S 14c2a0,4 miss 
L 10c0b4,4 hit 
S 14c320,4 miss 
L 10c0b8,4 hit 
```

所以A矩阵起始位置是0x10c0a0,B矩阵起始位置0x14c0a0，两个矩阵位置相差0x40000.

### AB矩阵与cache映射关系
  
> note:这里我们不以实际地址分析cache的映射关系，而是用相对地址。假设A[0][0]地址是0x00000，B[0][0]地址0x40000

A矩阵和B矩阵中的元素与cache的映射关系：

|A[0][0]~A[0][7]|A[0][8]~A[0][15]|A[0][16]~A[0][23]|A[0][24]~A[0][31]|
|----|----|----|----|
|line0|line1|line2|line3|
|...|...|...|...|
|A[7][0]~A[7][7]|A[7][8]~A[7][15]|A[7][16]~A[7][23]|A[7][24]~A[7][31]|
|line28|line29|line30|line31|
|...|...|...|...|
|A[31][0]~A[31][7]|A[31][8]~A[31][15]|A[31][16]~A[31][23]|A[31][24]~A[31][31]|
|line28|line29|line30|line31|

**32x32矩阵中每8行的元素会映射到同一个cache line**

### 对角线位置的缓存冲突不命中？

进一步地，A和B矩阵元素可以表示为：

```c
A[i][j] = A[i *32 + j]
B[j][i] = B[j* 32 + i]
```

同时访问A[i][j]和B[j][i]，两个元素位置相差：

```c
(base_a + i * 32 + j) - (base_b + j * 32 + i) 
= (base_a - base_b) + 31(i - j)
= 0x40000 + 31(i - j)

```

当i - j = 0时，A[i][j]和B[j][i]会映射到同一个cache line。如果两个元素一前一后被访问，会发生冲突不命中，举例说明：

- 访问A第一行B第一列：
  - 读A[0][0], 由于cache invalid, `cache miss`, A[0][0]~A[0][7]会映射到cache line 0；
  - 写B[0][0]时，`cache miss`后`eviction`前面的data 内容，将B[0][0]~B[0][7]映射到cache line 0
  - 读A[0][1], 也是到cache line 0寻找，`cache miss eviction`，将A[0][1]~A[0][7]会映射到cache line 0
  - 写B[1][0],到cache line 4中寻找，`cache miss`，将B[1][0]~B[1][7]会映射到cache line 4
  - 读A[0][2]~A[0][7],到cache line 0中寻找，`cache hit`
  - 写B[2][0]~B[7][0], `cache miss`

- 访问A第二行B第二列
  - 读[1][0],`cache miss eviction`, 将A[1][0]~A[1][7]会映射到cache line 4；(每行第一个元素都会miss)
  - 写B[0][1],`cache miss eviction`, 将B[0][0]~B[0][7]会映射到cache line 0(将A上一次占据的cache line抢过来，而A已经用不上这个line了)
  - 读A[1][1],`cache line 4 hit`；
  - 写B[1][1], `cache miss eviction`, 映射到cache line 4；(对角线冲突)
  - 读A[1][2],`cache miss eviction`；（对角线冲突）
  - 读A[1][3]~A[1][7] hit
  - 写B[2][1]~B[7][1], hit

**可以发现在访问A[i][i+1]时会多一次cache miss eviction, 访问B[i][i]和B[i -1][i]也会有cache miss eviction**

### cache miss=1183理论计算
  
- A矩阵按行遍历，遍历第i行时，A[i][0]~A[i][7] 会寻找第k个cache line,`cache miss`后会将A[i][0]~A[i][7] 换到cache line中。所以每行有32/8 = 4 cache miss/row, 其余hit，总cache miss数量 = 32 row *4 miss/row= 128 misses。but注意在上面分析过访问A[i][i+1]时会多一次cache miss, 所以实际上cache miss = 32* 4 + 31 = 128 + 31 = 159

- B矩阵按列遍历，遍历第i(i % 8 = 0)列时， 访问B[j][i]都会发生cache miss, 然后将B[j][i]~B[j][i+7]按行取8个元素写入cache line。在分析矩阵和cache line映射关系时已知：访问到B[j + 8 *n][i]时会和B[j][i]映射到同一个cache line，所以当B遍历到i+1行时还是会发生cache miss。因此每列都有32 个cache miss，总cache miss数量 = 32* 32 = 1024

  total cache miss数量 = 159 + 1024 = 1183

### 分块优化

- 重点是减少B矩阵cache miss数量，尽可能使用cache line中的数据
- 使用局部变量存储A矩阵的元素，避免对角线cache冲突
- 按照hints中提到的使用分块技术
  - 分块大小多少比较合适？
    按照cache line中data block恰好能放8个int，以及每隔8行元素会映射同一个cache line,所以分块大小为8 * 8。理论上，这样在访问B[j][i+1]时，不会发生cache miss。

#### 优化1

```c

void trans_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ii, jj;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (ii = 0; ii < 8; ii++) {
                for (jj = 0; jj < 8; jj++) {
                    int row     = ii + i;
                    int col     = jj + j;
                    B[col][row] = A[row][col];
                }
            }
        }
    }
}
```

测试结果：`func 0 (Transpose submission): hits:1709, misses:344, evictions:312`

**分析cache misses = 344**

32x32矩阵别划分为16个8x8的块

- 对于`非对角线块`，A矩阵遍历每一行会发生一个cache miss, B矩阵遍历第一列会cache miss，第2~8列cache hit。所以非对角线块发生cache miss数量 = (8 + 8) * 12 = 192
- 对于`对角线块`，A矩阵和B矩阵访问这个块时每行都有缓存冲突，假设它们访问的都是第一个块，这个块的8行分别映射的是line0,4,8,12,16,20,24,28。
  - 在上面分析过访问A[i][i+1]时会多一次cache miss, 在一个8*8 block中，A由于对角线冲突导致的总cache miss = 8(每行一开始的cache miss) + 7(对角线冲突)= 15 misses/block
  - B访问每个block的第一列元素都会cache miss, 加上对角线冲突，B[i][i]和B[i-1][i]会cache miss。因此B由于对角线冲突导致的总cache miss = 8(写B[i][x]) + 7(B[i][i]except 第一个元素) + 7(B[i-1][i]) = 22 misses/block
  - 依次类推，15 + 22 = 37 misses/block
  - 对角线块发生cache miss数量 = 4 * 37 = 148
- total misses = 192 + 148 = 340

> 多出来的 4 次是其他调用的开销、固定支出。

#### 优化2

减少对角线冲突，所以需要将A block每行的元素存储在局部变量中

- 对于非对角线块，非对角线块发生cache miss数量 = (8 + 8) * 12 = 192
- 对于对角线块:
  - A访问每一行第一个元素都会cache miss
  - B访问第一列每个元素都会cache miss，访问第n(n>1)列时，只有B[n][n]会cache miss
  - 对角线块发生cache miss数量 = (9 + 2 x 7) x 4 = 92
- total misses = 192 + 92 = 284

从附录hitf文件中可以进一步验证猜想。多出来的4次或者3次miss是其他调用的开销、固定支出。

```txt
S 18d08c,1 miss 
L 18d0a0,8 miss
......
S 18d08d,1 miss eviction 
```

```c
void trans_32x32_v2(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = 0; k < 8; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;
                B[j + 4][row] = a_4;
                B[j + 5][row] = a_5;
                B[j + 6][row] = a_6;
                B[j + 7][row] = a_7;
            }
        }
    }
}
```

`func 0 (Transpose submission): hits:1766, misses:287, evictions:255`

## 64x64优化思路

64x64矩阵如果被分割为8x8block,一共64个block，还用上述的优化方式，前8行对应的cache映射关系：

|.|8int|8int|8int|8int|8int|8int|8int|8int|
|----|----|----|----|----|----|----|----|----|
|row0|line 0|line 1|line 2|line 3|line 4|line 5|line 6|line 7|
|row1|line 8|line 9|line 10|line 11|line 12|line 13|line 14|line 15|
|row2|line 16|line 17|line 18|line 19|line 20|line 21|line 22|line 23|
|row3|line 24|line 25|line 26|line 27|line 28|line 29|line 30|line 31|
|row4|line 0|line 1|line 2|line 3|line 4|line 5|line 6|line 7|
|row5|line 8|line 9|line 10|line 11|line 12|line 13|line 14|line 15|
|row6|line 16|line 17|line 18|line 19|line 20|line 21|line 22|line 23|
|row7|line 24|line 25|line 26|line 27|line 28|line 29|line 30|line 31|

重复上面的cache映射关系x8

### 理论分析8x8cache miss

用32x32优化2测试64 x 64 矩阵，结果如下：
`func 0 (Transpose submission): hits:3585, misses:4612, evictions:4580`

- 在32x32优化2方式中，B矩阵按列遍历时，一个block中访问每一行映射的都是不同的cacheline，而现在row4~row7与row0~row3的cacheline是同一个cacheline，所以B矩阵的访问会反复的cache trash(抖动)。B矩阵cache miss数量 = 8 x 8 x 64  = 4096.
- A矩阵按照优化2的计算结果，cache miss = 8 x 64 = 512
- total cache miss = 4096 + 512 = 4608. 加上4个系统调用开销，总cache miss = 4608 + 4 = 4612

### 优化1

从上述分析过程，有以下优化思路：

- A cache miss数量不受影响，主要是B矩阵的cache 抖动占大部分原因。如何优化？
  - B遍历block不能8行遍历，前4行存到cache line中的数据要在写后四行之前就使用。可是读到A一行8个元素时，需要写到B的列中，如果不写后4行可以放在哪里呢？临时变量个数限制了不能全部放局部变量，不妨放到B[j][i + 4]中，不会造成多的cache miss.  
  - 接下来A3 A4怎么搬到B中呢？临时变量暂存A2_t, 读A3写入B2，B2中原本存放的A2_t要写入B3. 最后搬运A4到B4中。
  
  按照上述的优化思路得到的数据分布情况：

  A block按4 x 4分块：

  |A1|A2|
  |----|----|
  |A3|A4|

  期望得到的B block如下,Ax_t是Ax的转置矩阵（感觉有点像行列式呢）

  |A1_t|A3_t|
  |----|----|
  |A2_t|A4_t|

  在上述优化后，B矩阵的存储变化为：

  |A1_t|A2_t|
  |----|----|
  |NULL|NULL|
  
  |A1_t|A2_t|
  |----|----|
  |A3_t|A4_t|

  |A1_t|A3_t|
  |----|----|
  |A2_t|A4_t|

在exchange A2_t和A3_t会有cache冲突

```c
void trans_64x64_v1(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    for (i = 0; i < 64; i += 8) {
        for (j = 0; j < 64; j += 8) {
            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                // B 1 = A1_t
                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;
                // B2 = A2_t
                B[j + 0][row + 4] = a_4;
                B[j + 1][row + 4] = a_5;
                B[j + 2][row + 4] = a_6;
                B[j + 3][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row + 4][j + 0];
                int a_1 = A[row + 4][j + 1];
                int a_2 = A[row + 4][j + 2];
                int a_3 = A[row + 4][j + 3];
                int a_4 = A[row + 4][j + 4];
                int a_5 = A[row + 4][j + 5];
                int a_6 = A[row + 4][j + 6];
                int a_7 = A[row + 4][j + 7];

                // B3 = A3_t
                B[j + 4][row] = a_0;
                B[j + 5][row] = a_1;
                B[j + 6][row] = a_2;
                B[j + 7][row] = a_3;
                // B4 = A4_t
                B[j + 4][row + 4] = a_4;
                B[j + 5][row + 4] = a_5;
                B[j + 6][row + 4] = a_6;
                B[j + 7][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = j + k;

                // B2 = A2_t
                int a_0 = B[row][i + 4];
                int a_1 = B[row][i + 5];
                int a_2 = B[row][i + 6];
                int a_3 = B[row][i + 7];

                // B3 = A3_t
                int a_4 = B[row + 4][i + 0];
                int a_5 = B[row + 4][i + 1];
                int a_6 = B[row + 4][i + 2];
                int a_7 = B[row + 4][i + 3];

                // B2 = A3_t
                B[row][i + 4] = a_4;
                B[row][i + 5] = a_5;
                B[row][i + 6] = a_6;
                B[row][i + 7] = a_7;

                // B2 = A2_t
                B[row + 4][i + 0] = a_0;
                B[row + 4][i + 1] = a_1;
                B[row + 4][i + 2] = a_2;
                B[row + 4][i + 3] = a_3;
            }
        }
    }
}

```

测试结果：
`func 0 (Transpose submission): hits:10193, misses:2100, evictions:2068`

**cache miss = 2100 分析？**

对角线：
8 Amiss + 8 Bmiss + 6(读B[i][i]miss) + exchange(2 read Bmiss + 2 load Bmiss) * 4 rows = 38 miss/bloc

非对角线：
8 + 8 + (2+2) * 4 = 32 miss/block

38 * 8 block + 32 * 56 block = 2096 misses


### 优化2

```c
void trans_64x64_v2(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    for (i = 0; i < 64; i += 8) {
        for (j = 0; j < 64; j += 8) {
            for (k = 0; k < 4; k++) {
                int row = i + k;

                int a_0 = A[row][j + 0];
                int a_1 = A[row][j + 1];
                int a_2 = A[row][j + 2];
                int a_3 = A[row][j + 3];
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                // B1 = A1_t
                B[j + 0][row] = a_0;
                B[j + 1][row] = a_1;
                B[j + 2][row] = a_2;
                B[j + 3][row] = a_3;

                // B2 = A2_t
                B[j + 0][row + 4] = a_4;
                B[j + 1][row + 4] = a_5;
                B[j + 2][row + 4] = a_6;
                B[j + 3][row + 4] = a_7;
            }

            for (k = 0; k < 4; k++) {
                int row = j + k;

                // B2 = A2_t
                int a_0 = B[row][i + 4];
                int a_1 = B[row][i + 5];
                int a_2 = B[row][i + 6];
                int a_3 = B[row][i + 7];

                // get A3_t
                int a_4 = A[i + 4][row];
                int a_5 = A[i + 5][row];
                int a_6 = A[i + 6][row];
                int a_7 = A[i + 7][row];

                // B2 = A3_t
                B[row][i + 4] = a_4;
                B[row][i + 5] = a_5;
                B[row][i + 6] = a_6;
                B[row][i + 7] = a_7;

                // B3 = A2_t
                B[row + 4][i + 0] = a_0;
                B[row + 4][i + 1] = a_1;
                B[row + 4][i + 2] = a_2;
                B[row + 4][i + 3] = a_3;
            }

            for (k = 0; k < 4; k++) {
                int row = i + 4 + k;
                int a_4 = A[row][j + 4];
                int a_5 = A[row][j + 5];
                int a_6 = A[row][j + 6];
                int a_7 = A[row][j + 7];

                B[j + 4][row] = a_4;
                B[j + 5][row] = a_5;
                B[j + 6][row] = a_6;
                B[j + 7][row] = a_7;
            }
        }
    }
}
```

## 61x67优化思路

尝试了多个block size都能满足要求

```c
void trans_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, n;
    int block = 20;   // 17,18,19 20 are OK
    for (i = 0; i < 61; i += block) {
        for (j = 0; j < 67; j += block) {
            for (k = i; k < i + block && k < 61; k++) {
                for (n = j; n < j + block && n < 67; n++) {
                    B[n][k] = A[k][n];
                }
            }
        }
    }
}
```

block = 18测试结果

`func 0 (Transpose submission): hits:6206, misses:1973, evictions:1941`

# 遇到的问题

- index\tag数据类型写太小，导致寻找cache set错误
- 解析trace file 格式错误，导致解析结果很奇怪
- 矩阵转置优化好难(哭). 分析完32x32就有点累了，64x64优化2参考了网上[别人的实现](https://zhuanlan.zhihu.com/p/484657229)，61x67纯属瞎试验的，结果好几个测试结果都满足要求
- 增加makefile clean target对应的删除目标,删除一下编译产物
  ```makefile
	rm -f *.tmp hitf
  ```

# 附录

32x32优化2 hift文件：

```txt
S 18d08c,1 miss 
L 18d0a0,8 miss 
L 18d084,4 hit 
L 18d080,4 hit 

L 10d080,4 miss eviction A[0][0]
L 10d084,4 hit 
L 10d088,4 hit 
L 10d08c,4 hit 
L 10d090,4 hit 
L 10d094,4 hit 
L 10d098,4 hit 
L 10d09c,4 hit 

S 14d080,4 miss eviction B[0][x]
S 14d100,4 miss 
S 14d180,4 miss 
S 14d200,4 miss 
S 14d280,4 miss 
S 14d300,4 miss 
S 14d380,4 miss 
S 14d400,4 miss 

L 10d100,4 miss eviction A[1][0]
L 10d104,4 hit 
L 10d108,4 hit 
L 10d10c,4 hit 
L 10d110,4 hit 
L 10d114,4 hit 
L 10d118,4 hit 
L 10d11c,4 hit 

S 14d084,4 hit B[0][1]
S 14d104,4 miss eviction B[1][1]
S 14d184,4 hit 
S 14d204,4 hit 
S 14d284,4 hit 
S 14d304,4 hit 
S 14d384,4 hit 
S 14d404,4 hit 

L 10d180,4 miss eviction  A[2][0]
L 10d184,4 hit 
L 10d188,4 hit 
L 10d18c,4 hit 
L 10d190,4 hit 
L 10d194,4 hit 
L 10d198,4 hit 
L 10d19c,4 hit 

S 14d088,4 hit 
S 14d108,4 hit 
S 14d188,4 miss eviction B[2][2]
S 14d208,4 hit 
S 14d288,4 hit 
S 14d308,4 hit 
S 14d388,4 hit 
S 14d408,4 hit 

L 10d200,4 miss eviction A[0][3]
L 10d204,4 hit 
L 10d208,4 hit 
L 10d20c,4 hit 
L 10d210,4 hit 
L 10d214,4 hit 
L 10d218,4 hit 
L 10d21c,4 hit 

S 14d08c,4 hit 
S 14d10c,4 hit 
S 14d18c,4 hit 
S 14d20c,4 miss eviction B[3][3]
S 14d28c,4 hit 
S 14d30c,4 hit 
S 14d38c,4 hit 
S 14d40c,4 hit 

L 10d280,4 miss eviction A[0][4]
L 10d284,4 hit 
L 10d288,4 hit 
L 10d28c,4 hit 
L 10d290,4 hit 
L 10d294,4 hit 
L 10d298,4 hit 
L 10d29c,4 hit 

S 14d090,4 hit 
S 14d110,4 hit 
S 14d190,4 hit 
S 14d210,4 hit 
S 14d290,4 miss eviction B[4][4]
S 14d310,4 hit 
S 14d390,4 hit 
S 14d410,4 hit 

L 10d300,4 miss eviction A[0][5]
L 10d304,4 hit 
L 10d308,4 hit 
L 10d30c,4 hit 
L 10d310,4 hit 
L 10d314,4 hit 
L 10d318,4 hit 
L 10d31c,4 hit 

S 14d094,4 hit 
S 14d114,4 hit 
S 14d194,4 hit 
S 14d214,4 hit 
S 14d294,4 hit 
S 14d314,4 miss eviction B[5][5]
S 14d394,4 hit 
S 14d414,4 hit 

L 10d380,4 miss eviction A[0][6]
L 10d384,4 hit 
L 10d388,4 hit 
L 10d38c,4 hit 
L 10d390,4 hit 
L 10d394,4 hit 
L 10d398,4 hit 
L 10d39c,4 hit 

S 14d098,4 hit 
S 14d118,4 hit 
S 14d198,4 hit 
S 14d218,4 hit 
S 14d298,4 hit 
S 14d318,4 hit 
S 14d398,4 miss eviction B[6][6]
S 14d418,4 hit 

L 10d400,4 miss eviction A[0][7]
L 10d404,4 hit 
L 10d408,4 hit 
L 10d40c,4 hit 
L 10d410,4 hit 
L 10d414,4 hit 
L 10d418,4 hit 
L 10d41c,4 hit 

S 14d09c,4 hit 
S 14d11c,4 hit 
S 14d19c,4 hit 
S 14d21c,4 hit 
S 14d29c,4 hit 
S 14d31c,4 hit 
S 14d39c,4 hit 
S 14d41c,4 miss eviction B[7][7]
```
