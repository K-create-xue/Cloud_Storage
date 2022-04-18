# Cloud_Storage
云备份 项目


# 项目介绍：
## 客户端功能介绍：
  在客户端下生成文件scandir  在文件client_util.hpp  文件第235行中#define  定义可自行修改
  还生成文件data.conf  在文件   client_util.hpp  文件第236行中#define  定义可自行修改
  scandir为监控文件，客户端将每隔1s（可设置，在client_util.hpp 314行 修改usleep时间即可） 对文件进行扫描
  判断有无文件发生变化（利用最后一次修改时间判断），发生改变，自动上传到服务端，
  服务端ip和端口可以在client_cloud.cpp 中进行设置；

## 服务端功能介绍：
  自动创建文件backup （存储上传文件）  、 packdir（压缩后文件存储） 、backup.conf（记录文件存储信息，![0EC088E8](https://user-images.githubusercontent.com/76169472/163791128-42574ac4-b016-48b4-820a-3ceff193b224.png)
）

    在文件backup.conf 文件中记录文件信息
    未压缩文件记录：
      filename.txt=filename.txt
    压缩文件记录：
      filename.txt=./packdir/filename.txt.pack
 
 两个线程并发执行，一个监控packup文件下文件是否为非热点文件，是则压缩；
 另一个作为服务器监听客户端的请求；
 
 客户端通过ip+端口/list  查看当前服务器packdir中文件，可点击下载操作（支持断点续传操作）
 断点续传可在我的csdn： http://t.csdn.cn/8Y7l6 中查看具体技术实现；

# 使用方法：

## 客户端文件：
client_util.hpp
client_cloud.cpp
httplib.h

运行之后会在当前路径下创建一个scandir 文件夹，
程序每间隔1s会监控一次文件夹下文件，
判断是否有文件发生改变（最后一次修改时间），
有修改就自动上传存储。
在client_cloud.cpp里面设置服务器的ip和端口即可。


## 服务端文件：
其他文件都为服务端文件包括httplib.h
全部下载后：
生成的可执行程序为为cloud；
运行后会在当前路径下创建文件夹backup用来保存上传未压缩文件；
还会创建一个packdir文件夹存放压缩后文件
压缩文件根据util.hpp 文件里面的451行的  _hot_time 来判断是否为非热点文件
（即经过 _hot_time 时间之后文件没有被人访问过，即为非热点文件）
就可以对文件进行压缩；

例如：
filename.txt   压缩后   filename.txt.pack

