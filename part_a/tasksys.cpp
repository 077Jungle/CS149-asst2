#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    threads = new std::thread[num_threads];
    num_threads_ = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] threads;
}

void threadRun(IRunnable* runnable, int num_total_tasks, std::mutex *mutex, int *count)
{
    int done = 0;
    while (done < num_total_tasks)
    {
        mutex->lock();
        done = *count;
        *count += 1;
        mutex->unlock();
        if (done < num_total_tasks)
            runnable->runTask(done, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    int count = 0;
    std::mutex *mutex = new std::mutex();
    for (int i = 0; i < num_threads_; i++)
    {
        threads[i] = std::thread(threadRun, runnable, num_total_tasks, mutex, &count);
    }
    for (int i = 0; i < num_threads_; i++)
    {
        threads[i].join();
    }
    delete mutex;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

ThreadState::ThreadState()
{
    mutex_ = new std::mutex();
    finished_ = new std::condition_variable();
    todo_number = -1;
    done_number = -1;
    total_number = -1;
}

ThreadState::~ThreadState()
{
    delete mutex_;
    delete finished_;
}

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = false;
    threads = new std::thread[num_threads];
    num_threads_ = num_threads;
    state_ = new ThreadState();
    for (int i = 0; i < num_threads_; i++)
        threads[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::threadRun, this);
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    killed = true;
    for (int i = 0; i < num_threads_; i++)
        threads[i].join();
    delete[] threads;
    delete state_;
}

void TaskSystemParallelThreadPoolSpinning::threadRun()
{
    int todo, total;
    while (!killed)
    {
        state_->mutex_->lock();
        todo = state_->todo_number;
        total = state_->total_number;
        if (todo < total)
            state_->todo_number++;
        state_->mutex_->unlock();
        if (todo < total)
        {
            state_->runnable_->runTask(todo, total);
            state_->mutex_->lock();
            state_->done_number++;
            state_->mutex_->unlock();
            if (state_->done_number == total)
            {
                state_->finished_->notify_one();
            }
        }
    }
};

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*state_->mutex_);

    state_->todo_number = 0;
    state_->done_number = 0;
    state_->total_number = num_total_tasks;
    state_->runnable_ = runnable;

    while (state_->done_number != num_total_tasks) {
        state_->finished_->wait(lk);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

ThreadStateCondition::ThreadStateCondition()
{
    mutex_ = new std::mutex();
    done_mutex_ = new std::mutex();
    finished_ = new std::condition_variable();
    todo_number = -1;
    done_number = -1;
    total_number = -1;
}

ThreadStateCondition::~ThreadStateCondition()
{
    delete mutex_;
    delete done_mutex_;
    delete finished_;
}

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = false;
    num_threads_ = num_threads;
    has_ = new std::condition_variable();
    state_ = new ThreadStateCondition();
    threads = new std::thread[num_threads];
    for (int i = 0; i < num_threads; i++)
        threads[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::threadRun, this);
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = true;
    has_->notify_all();
    for (int i = 0; i < num_threads_; i++)
        threads[i].join();
    delete has_;
    delete state_;
    delete[] threads;
}

void TaskSystemParallelThreadPoolSleeping::threadRun()
{
    int todo, total;
    while (!killed)
    {
        std::unique_lock<std::mutex> lk(*state_->mutex_);
        while (state_->todo_number == state_->total_number && !killed) {
            has_->wait(lk);
        }

        todo = state_->todo_number;
        total = state_->total_number;
        if (todo < total)
            state_->todo_number++;
        lk.unlock();
        if (todo < total)
        {
            state_->runnable_->runTask(todo, total);
            state_->mutex_->lock();
            state_->done_number++;
            state_->mutex_->unlock();
            if (state_->done_number == total) {
                // adding done_mutex can accelerate performance 
                // when run finishes former task and locks the done_mutex, this thread will be blocked here 
                // run task modifies task configuration so this thread needn't to wait
                state_->done_mutex_->lock();
                state_->done_mutex_->unlock();
                state_->finished_->notify_one();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*state_->done_mutex_);

    state_->mutex_->lock();
    state_->todo_number = 0;
    state_->total_number = num_total_tasks;
    state_->done_number = 0;
    state_->runnable_ = runnable;
    state_->mutex_->unlock();

    has_->notify_all();

    while (state_->done_number != num_total_tasks)
        state_->finished_->wait(lk);
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
