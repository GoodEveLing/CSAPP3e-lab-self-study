# 任务需求
- partA: 实现一个cache模拟器
- partB: 优化一个矩阵转置函数,目标是最小化cache misses数量
## 如何测试？

- 测试单个文件
```unix
./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
```
- 测试csim-ref和自己的csim
```unix
make && ./test-csim
```


# partA: 实现一个cache模拟器

## 前置知识
- cache结构
- 如何解析address以找到缓存块?
  - address = tag + index + offset = m bits
  |tag|set index|block offet|
  |----|----|----|
  |t bits|s bits|b bits|

- 缓存命中与不命中？
- cache miss的种类和原因?
- cache miss怎么处理?

## 核心实现
### 解析命令行输入参数
### init_cache
### 解析trace file
### find_cache


# partB: 优化一个矩阵转置函数

# 遇到的问题
- index\tag数据类型
- 解析trace file 格式错误