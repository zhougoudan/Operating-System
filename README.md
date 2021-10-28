# 小小操作系统 -基于xv6

这个项目起因是学完操作系统后,苦于没有项目练手,便自己动手写了一个小小的操作系统

受Linux系统启发,这个操作系统也可以通过一个shell来体现自己的存在

头文件可以通过xv6公开源码获得

## 此操作系统的功能
### 1 执行简单的命令

实现简单的命令. 这个shell可以实现:
1. 通过打印">>>"作为命令提示符提示用户输入命令
2. 执行输入到命令提示符中的命令
3. 程序无限循环直到shell退出
4. 处理cd命令
### 2 输入/输出重定向

实现输入/输出重定向. 这个shell可以实现:
1. 处理两个元素的重定向. 比如
~~~
echo "Hello world" > foo, or
cat < foo
~~~

### 3 管道
实现管道, 这个shell可以实现:
~~~
ls | grep mysh, or cat README | grep github
~~~

### 4 其他功能
1. 实现多元素管道,比如,
~~~
ls | head -3 | tail -1
~~~
2. 实现管道和重定向的组合,
~~~
ls | head -3 | tail -1 > myoutput
~~~
3. 实现“;”操作符，这个操作符可以顺序执行shell命令列表


## po一个当时运行截图
![20421634755086_ pic](https://user-images.githubusercontent.com/63355034/139286891-88d69c68-156c-49e0-a4cf-61711213e210.jpg)

