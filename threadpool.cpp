#include "threadpool.h"

// 线程池函数实现--------------------------------------------------------------

const int TASK_MAX_THRESHHOLD = 1024;

// 线程池构造
ThreadPool::ThreadPool() : initThreadSize_(0), taskSize_(0), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD), poolMode_(PoolMode::MODE_FIXED) {}

// 线程池析构
ThreadPool::~ThreadPool() {}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
    poolMode_ = mode;
}

// 设置初始线程数量
void ThreadPool::setInitThreadSize(int size)
{
    initThreadSize_ = size;
}

// 设置任务队列上限
void ThreadPool::setTaskQueMaxThreshHold(int threshHold)
{
    taskQueMaxThreshHold_ = threshHold;
}

// 提交任务给任务队列   用户调用该接口，传入对象生成任务
void ThreadPool::submitTask(std::shared_ptr<Task> task)
{
    // 获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    // 线程通信  等待任务队列有空余
    // 用户提交任务，最长不能阻塞超过1s，否则判断任务提交失败    wait   wait_for    wait_until
    if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]() -> bool
                           { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
    {
        // notFull等待一秒条件依然无法满足
        std::cerr << "task queue is full,submit task failed" << std::endl;
        return;
    }
    // 有空余，把任务放入任务队列
    taskQue_.emplace(task);
    taskSize_++;
    // 放了任务，任务队列不空了，在notEmpty_信号上通知,赶快分配线程执行任务
    notEmpty_.notify_all();
}

// 启动线程池
void ThreadPool::start(int initThreadSize)
{
    // 初始线程数量
    initThreadSize_ = initThreadSize;

    // 创建线程对象
    for (int i = 0; i < initThreadSize_; i++)
    {
        // 创建线程对象时，把用户需要执行的线程函数交给线程对象
        auto thread = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threads_.emplace_back(std::move(thread));

        // 启动所有线程
        threads_[i]->start();
    }
}

// 定义用户所需执行的线程函数,从任务队列消费任务
void ThreadPool::threadFunc()
{
    for (;;)
    {
        std::shared_ptr<Task> task;
        {
            // 先获取锁
            {
                std::unique_lock<std::mutex> lock(taskQueMtx_);
                // 等待notEmpty_条件
                notEmpty_.wait(lock, [&]() -> bool
                               { return taskQue_.size() > 0; });
                // 从任务队列中取一个任务出来
                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;
            }
            // 该线程执行任务前要释放锁，让其他线程可以获取锁,所以在这段代码加上一层中括号，出了中括号释放锁

            // 如果依然有其他任务，需要通知其他线程去取任务
            if (taskSize_ > 0)
            {
                notEmpty_.notify_all();
            }

            // 取出一个任务，需要通知,任务队列不满,可以继续提交生产任务
            notFull_.notify_all();
        }

        // 当前线程负责执行这个任务
        if (task != nullptr)
        {
            task->run();
        }
    }
}

// 线程函数实现--------------------------------------------------------------------

// 线程构造
Thread::Thread(ThreadFunc func) : func_(func)
{
}

// 线程析构
Thread::~Thread() {}

// 启动线程
void Thread::start()
{
    // 创建一个线程来执行用户绑定的线程函数
    std::thread t(func_);
    t.detach(); // 设置分离线程？
}