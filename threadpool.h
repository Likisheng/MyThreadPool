#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>

// Any:可以接受任意数据的类型
class Any
{
public:
    Any() = default;
    ~Any() = default;

    // 禁用左值拷贝构造跟赋值
    Any(const Any &) = delete;
    Any &operator=(const Any &) = delete;

    // 默认右值拷贝构造跟赋值
    Any(Any &&) = default;
    Any &operator=(Any &&) = default;
    template <typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data)){};

    template <typename T>
    T cast_() 
    {
        // 从基类指针找到派生类对象，取出data
        // 基类指针->派生类指针
        Derive<T> *pd = dynamic_cast<Derive<T> *>(base_.get());

        if (pd == nullptr)
        {
            throw("type is unmatch");
        }
        return pd->data_;
    }

private:
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    template <typename T>
    class Derive : public Base
    {
    public:
        Derive(T data) : data_(data)
        {
            
        };
        T data_;
    };

    // 定义一个基类指针
    std::unique_ptr<Base> base_;
};

// 实现一个信号量类
class Semaphore
{
public:
    Semaphore(int resLimit = 0) : resLimit_(resLimit){};
    ~Semaphore() = default;

    // 获取一个信号量资源
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待信号量有资源，没有资源阻塞当前线程
        cond_.wait(lock, [&]() -> bool
                   { return resLimit_ > 0; });
        resLimit_--;
    }

    // 增加一个信号量资源
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        cond_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};


//Task类前置声明
class Task;
// 实现接受提交到线程池的task任务执行完后的返回值类型Result
class Result
{
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);

    //获取任务执行完后的返回值，将其存储到any_
    void setVal(Any any);
    //用户调用该方法获取任务执行完后的返回值
    Any get();

private:
    Any any_;//存储任务返回值
    Semaphore sem_;//线程通信信号量，当任务还没结束时阻塞
    std::shared_ptr<Task> task_;//指向对应获取返回值的任务对象
    std::atomic_bool isValid_;//返回值是否有效
};

//  任务抽象基类
class Task
{
public:
    Task();
    ~Task() = default;
    void exec();
    void setResult(Result *res);
    // 纯虚函数，用户可以自定义任务类型，从Task继承，重写run方法
    virtual Any run() = 0;
private:
    Result *result_;//不能使用智能指针，会发生智能指针的交叉引用，永远不会释放内存
};

// 线程模式
enum class PoolMode
{
    MODE_FIXED, // 线程数量固定
    MODE_CACHED // 线程数量可动态增长
};

// 线程类型
class Thread
{
public:
    using ThreadFunc = std::function<void()>;
    // 线程构造
    Thread(ThreadFunc);

    // 线程析构
    ~Thread();

    // 启动线程
    void start();

private:
    // 存储用户需要线程执行的函数
    ThreadFunc func_;
};

// 线程池类型
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    // 禁止拷贝构造和拷贝赋值
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // 设置线程池工作模式
    void setMode(PoolMode mode);

    // 设置初始线程数量
    void setInitThreadSize(int size);

    // 设置任务队列上限
    void setTaskQueMaxThreshHold(int threshHold);

    // 提交任务给任务队列
    Result submitTask(std::shared_ptr<Task>);

    // 启动线程池
    void start(int initThreadSize = 8);

private:
    // 线程函数，用户传入的需要多线程执行的函数
    void threadFunc();

private:
    std::vector<std::unique_ptr<Thread>> threads_; // 线程列表
    size_t initThreadSize_;                        // 初始线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
    std::atomic_int taskSize_;                  // 任务数量
    int taskQueMaxThreshHold_;                  // 任务队列数量最大值

    std::mutex taskQueMtx_;            // 保证任务队列的线程安全
    std::condition_variable notFull_;  // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空

    PoolMode poolMode_; // 当前线程池工作模式
};

#endif