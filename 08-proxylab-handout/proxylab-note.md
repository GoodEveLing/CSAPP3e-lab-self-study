# 任务需求

写一个简单的能够缓存网络对象的HTTP代理服务器，。

- part1: 建立代理服务器，用于接收连接，读和解析请求，并转发请求到目标服务器，读并将目标服务器的响应转发给客户端。这个part包含学习基础HTTP操作和如何使用sockets来建立网络连接。
- part2: 升级代理以处理多个并发连接。这将向您介绍如何处理并发，这是一个关键的系统概念。
- part3: 将使用最近访问的简单主内存缓存将缓存添加到您的代理Web 内容。

## internet connection

- 套接字是什么？
  从程序来看是一个有相应描述符的打开文件
- 一个连接和套接字的关系？
- 套接字接口常用的有哪些？
  connect: client connects to server
  bind: 告诉内核将addr中的服务器套接字地址和套接字描述符sockfd联系起来
  listen: 服务器监听来自客户端的连接请求
  accept: 服务器等待来自客户端的连接请求
- 监听描述符和已连接描述符的区别？
- getaddrinfo 将一串字符串转化为套接字地址结构
- getnameinfo 将套接字地址转换为主机和服务名字符串
- open_clientfd 客户端调用这个建立与服务器的连接
- open_listenfd 服务器调用来创建一个监听描述符，准备好连接请求

## autograding

```unix
make 

./driver.sh
```

# partI - 实现顺序网络代理

> 实现一个基础的顺序代理，能够处理HTTP/1.0 GET 请求。启动时，您的代理应在命令行指定的端口上监听传入的连接。一旦建立连接，您的代理应从客户端读取整个请求并解析请求。它应确定客户端是否发送了有效的HTTP请求；如果是，则可以建立与适当Web服务器的连接，然后请求客户端指定的对象。最后，您的代理应读取服务器的响应并将其转发给客户端。

- HTTP请求长啥样？
  `method URI verison`
  URI是URI的后缀，包括文件名和可选参数
- HTTP post组成？
  `verison status-code status message`
- 怎么解析HTTP request?
  将request解析为hostname + path / query
- proxy请求是以 HTTP /1.0 结尾，所以如果接收到的是HTTP /1.0那么需要修改
- rio是啥？
  robust IO 函数包


模仿tiny.c在


# partII - 处理多个并发请求

# partIII - 缓存web对象