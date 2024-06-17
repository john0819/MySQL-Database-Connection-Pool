#include "CommonConnectionPool.h"
#include "public.h"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
    // 静态对象的初始化，编译器自动进行lock和unlock
    // 第一次运行到其才进行初始化（懒汉）
    static ConnectionPool pool;
    return &pool;
}

// 对配置文件进行加载到连接池中的配置变量
bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.ini", "r");
    if (pf == nullptr) {
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while(!feof(pf)) 
    {
        char line[1024] = { 0 };
        fget(line, 1024, pf);
        string str = line;
        int idx = str.find('=', 0);
        if(idx == -1) // 无效的配置项
        {
            continue; 
        }

        // password=123456\n
        int endidx = str.find('\n', idx); // password
		string key = str.substr(0, idx); // 123456
		string value = str.substr(idx + 1, endidx - idx - 1);

        if(key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
        else if (key == "dbname")
		{
			_dbname = value;
		}
        else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}

    }

    return true;
}

// 连接池的构造
ConnectionPool::ConnectionPool() 
{
    // 加载配置项
    if(!loadConfigFile())
    {
        return;
    }

    // 创建初始数量的连接
    // 系统启动的时候，所以不需要考虑 线程安全问题
    // 运行以后需要考虑对_connectionQue 线程安全问题
    for(int i=0; i < initSize; ++i) {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); // 刷新开始空闲的起始时间
        _connectionQue.push(p);
        _connectionCnt++; 
    }

    // 启动一个新的线程，作为连接生产者，创建新的连接池中的mysql
    // Linux thread 底层调用为 pthread_create
    // 成员方法作为线程函数，其为C接口，需要绑定器 绑定 this对象
    thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach(); // 分离线程，守护线程

    // 启动一个新的定时线程，扫描多余的空闲连接，超过maxIdleTime时间的空闲连接，进行连接回收
    thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();

}

// 运行在独立的线程中，专门负责生成新连接
// 当发现队列为空的时候，需要生成连接mysq
// ** 描述清楚生产者线程 **
void ConnectionPool::produceConnectionTask() 
{
    // 不断进行查看
    for(;;) 
    {
        unique_lock<mutex> lock(_queueMutex);
        while(!_connectionQue.empty())
        {
            // 为空的时候，消费者会进行唤醒
            // 如果进入睡眠模式，会释放锁，让消费者线程进行获取实例
            cv.wait(lock); // 队列不空，此处生产线程进入等待状态
        }

        // 连接数量没有达到上限，继续创建新的连接
        if(_connectionCnt < _maxSize) 
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // 通知消费者线程，可以进行消费连接（取mysql）
        cv.notify_all();
    }

}

// 消费者线程 提供给外部进行调用接口
shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    // 通过while来避免唤醒后被其他线程抢占资源问题
    while (_connectionQue.empty())
    { 
        // 等待连接最长时间，判断是否能获取mysql实例
        // 有可能其他线程归还，或者生产者创建新的
        // 此处需要考虑，唤醒后是否会被其他线程抢占的问题
        if(cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
        {
            if(_connectionQue.empty())
            { 
                LOG("获取空闲连接超时... 获取连接失败!");
                return nullptr;
            }
        }
    }

    /*
        shared_ptr析构时，会把connection资源直接进行delete
        相当于调用connection的析构函数，connection被close
        需要自定义shared_ptr释放资源的方式，把connection归还到queue中
        进行自定义删除器 lambda

        通过智能指针：避免用户在使用过程中忘记归还
    */

    shared_ptr<Connection> sp(_connectionQue.front(),
        [&](Connection *pcon) {
            // 此处为服务器应用线程中调用，需要考虑队列线程安全问题
            unique_lock<mutex> lock(_queueMutex);
            pcon->refreshAliveTime();
            _connectionQue.push(pcon);
        });

    _connectionQue.pop();
    // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
    cv.notify_all();
    return sp;
}

void ConnectionPool::scannerConnectionTask()
{
    for(;;)
    {
        // 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余的连接
        unique_lock<mutex> lock(_queueMutex);
        while(_connectionCnt > initSize) 
        {
            Connection *p = _connectionQue.front();
            if(p->getAliveTime() > (maxIdleTime*1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // 释放连接，调用~Connection()
            }
            else
            {
                break; // 队头一定是最大的空闲时间
            }
        }
    }
}


