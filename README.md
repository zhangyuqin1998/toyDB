# webserver1.0
最近在工作中使用bazel比较多，于是就趁热用bazel构建了这个webserver项目

参照游双《Linux高性能服务器编程》，实现了这个小玩具

目前实现的功能：

* epoll多路复用IO，使用半同步半反应堆模式
* C++11风格的线程池，使用条件变量实现阻塞队列，利用std::function和std::bind将任务函数绑定添加到线程池
* 利用SIGALRM实现时间堆，关闭长时间不活跃的连接
* 使用了一丢丢模板，正在学元编程的一些技法，看看之后能不能加入一些元编程的部分
