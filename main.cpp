#include "threadpool.h"
#include <chrono>

using ULong = unsigned long long;

class MyTask : public Task
{
public:

    MyTask(int begin, int end):begin_(begin), end_(end){}
    Any run()
    {
        std::cout << "tid:" << std::this_thread::get_id() << "begin!" << std::endl;
        ULong sum = 0;
        for (int i = begin_; i < end_; i++)
        {
            sum += i;
        }
        std::cout << "tid:" << std::this_thread::get_id() << "end!" << std::endl;
        return sum;
    }

private:
    int begin_;
    int end_;
};

int main()
{
    ThreadPool pool;
    pool.start(16);

    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(100001, 200000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(200001, 300000));

    ULong sum1 = res1.get().cast_<ULong>();
    ULong sum2 = res2.get().cast_<ULong>();
    ULong sum3 = res3.get().cast_<ULong>();

    std::cout << "Result is " << (sum1 + sum2 + sum3) << std::endl;

    return 0;
}