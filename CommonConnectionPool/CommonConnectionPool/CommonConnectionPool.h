/*

    实现连接池模块 设计成单例模式
    并且需要保证线程安全（多线程模式下）

*/

#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <memory>
#include <functional>
using namespace std;
#include "Connection.h"

class ConnectionPool
{
public:
    // 获取连接池对象实例
    static ConnectionPool* getConnectionPool();
    // 给外部提供接口，从连接池中获取一个可用的空闲连接，提供给消费者线程
    // 通过智能指针，重定义删除器:将出作用域（不使用）归还回queue中
    // 避免使用backQueue的操作
    shared_ptr<Connection> getConnection();
private:
    // 单例 构造函数私有化
    ConnectionPool();

    // 从配置文件中加载配置项
    bool loadConfigFile();

    // 运行在独立的线程中，专门负责生成新连接
    void produceConnectionTask();

    // 运行定时线程，对空闲连接进行检查是否进行连接回收
    void scannerConnectionTask();

    // 数据库连接需要: ip地址 端口号 登录用户名 密码
    // 从配置文件中获取 （方便修改）
    string _ip; 
    unsigned short _port; // 默认3306
    string _username;
    string _password;
    string _dbname;

    int _initSize; // 连接池初始连接量
    int _maxSize; // 连接池最大连接量
    int _maxIdleTime; // 连接池最大空闲时间
    int _connectionTimeout; // 连接池获取连接超时时间

    // 从队列中取mysql连接 需要保证线程安全
    queue<Connection*> _connectionQue; // 存储mysql连接的队列
    mutex _queueMutex; // 维护连接队列的线程安全互斥锁
    // 是一个线程安全的++操作（原子性） 多进程下进行++
    atomic_int _connectionCnt; // 记录所创建的connection连接总数量
    condition_variable cv; // 设置条件变量，用于连接生产线程和消费线程的通信

};