# localq
Local Queue - 高性能本地队列

## Installation
```shell
make && make client
```
## Requirement
```shell
epoll
```

## Usage
```shell
1. push数据到队列
a) cd client; ./client
b) 在命令行输入任何想要push的字符串（不包括 status 和 quit）
*************************************
+               Localq              +
*************************************
最大工作线程数量	2
当前工作线程数量	1
历史创建线程数量	1
历史退出线程数量	0
当前已处理请求量	5
处理失败请求数量	5
处理请求累计时间	0天0时0分0秒
处理请求最长时间	0.000001
处理请求最短时间	0.000001
处理请求平均时间	0.000001
服务已经运行时长	0天0时3分12秒
当前队列请求数量	0
当前队列存储容量	0M
当前队列容量上限	100M
当前连接数量上限	101
当前建立连接数量	2
历史建立连接数量	3
历史关闭连接数量	1


> hello
ps: push "hello" 到队列， status显示当前server状态， quit退出当前client
```
