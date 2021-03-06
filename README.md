这是一个分布式的**kv缓存系统**

支持两种模式：

1. 单机模式。基于client/server模式，使用Protobuf通信，可直接使用client操作或在golang\c++中导入客户端的包来进行调用
2. 多机模式。使用类似服务层-业务层-数据层架构，无需client，直接使用HTTP通信。是一个中心化的分布式系统，由一个统一的服务层进程管理多机的数据节点。

为何使用**bazel**？快速构建、支持构建多种语言、便于扩展、结构清晰

为何使用**protobuf**？性能优秀、支持多种编程语言、简洁易懂


（代码不是最新，有一些还没调试完）
代码结构：
```
toyDB-+-server-+-BUILD
      |        |-c++服务端内核
      |-client-+-BUILD
      |        |-c++客户端
      |        |-go客户端
      |-api-+-BUILD
      |     |-dealHeartbeat处理挂掉的节点
      |     |-dealLocate选择节点
      |     |-forward转发请求
      |     |-apiServer.go主函数
      |-httphandler-+-BUILD
      |             |-handler处理请求
      |             |-heartbeat发送心跳
      |-database-+-BUILD
      |          |-kv数据库组件
      |-protoc-+-BUILD
      |        |-通信协议
      |-test-+-BUILD
      |      |-测试用例
      |-WORKSPACE
      +-bazel-bin生成的可执行文件
```

TODO构建成分布式kv数据库(网络部分使用golang构建):
* 这是一个中心化的分布式系统(相对易于构建)
* toydb使用golang封装一个本地业务层接口，用来调用本地的数据层内核，作为httpHandler，执行命令并拿到结果。每台主机作为一个数据节点
* 封装一个服务层接口，管理分布式的数据节点。其中，数据节点定期发送heartbeat确认存活，服务层要及时将挂掉的节点踢出
* 用户的http请求发送到服务层，远端的服务层选择向特定的数据节点转发请求，数据节点在本地处理请求，将结果传回远端的服务层，服务层转发给客户。客户对分布式做到无感知

**目前实现的功能(内核部分使用C++构建)：**

* epoll多路复用IO，使用reactor模式
  * reactor + acceptor + handler
* C++11风格的线程池，使用条件变量实现阻塞队列，利用std::function和std::bind将任务函数绑定添加到线程池
* 利用SIGALRM和堆，关闭长时间不活跃的连接
* 使用protobuf作为序列化\反序列化工具，在server和client之间传递指
  * 实现了cc客户端和go客户端
* 基于LRU的缓存置换策略
* 添加了一些基于gtest的单元测试
* 基于std::unordered_map实现了简易的K-V存储系统，后期考虑增加多种存储结构
  * key为string
  * 使用ValueObject封装指向value的指针，目前可以作为value的类型有string、double、string_list(基于skiplist)、double_list
  * 使用union以节省空间，如果value是double或其它简单类型(和void\*所占空间相同)，就直接存储在union里，否则会存储指向value的指针。但要注意析构，避免内存泄露
  ```c
  /*database/toydb.h*/
  enum ValueType{
    NONE = 0,
    DOUBLE,
    STRING,
    DOUBLE_LIST,
    STRING_LIST,
  };
  
  class ValueObject{
  public:
    ...//省略了构造和析构函数
    ValueType value_type;
    union{
       double double_type;
       void* ptr_type; 
    }value;
  };
  ```
* K-V存储系统开放了insert_element, delete_element, has_element, get_element接口
  * 在client端分别使用`set key value`和`del key`和`has key`和`get key`来调用服务
  * server端执行相应的命令，并根据执行结果返回status
  * 若没有找到key, 会返回`NIL`, 插入成功返回`OK`
  * 若以列表作为value，列表中的全部值应为同一种类型，如：全为double或全为string
  * 若插入字符作为value，应被双引号`""`包围，否则暂认为是double
* 开放了接口用于向磁盘持久化数据和从磁盘读取数据
  * 启动server时可设置是否从磁盘读取数据
  * 定时向磁盘写入数据
  * 为后期的多机数据库主从复制做准备(主服务器启动时向从服务器发送自己的数据文件)


![1655221087(1)](https://user-images.githubusercontent.com/75946871/173618579-683e5389-ab6d-4ebd-98c1-94987b0b5dd6.png)


TODO:
* 增加持久化方法
* 仿照redis为存储系统建立更多的数据结构
   * 如，将sds引入项目，取代char[]
* 可以加入多种语言编写的客户端，如python、java、go
* 增加一些模板编程的技巧
* 目前仅支持std::string类型的key
* 加入垃圾回收机制
* 目前还是单机数据库，无论是稳定性还是存储能力都有明显的瓶颈，可以尝试往多机数据库方向发展
* 很多地方代码写的比较混乱，应该重构，并使用合适的设计模式
