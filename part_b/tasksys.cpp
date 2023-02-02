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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

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

    current_id = 0;
    running_task = 0;

    run_mutex = new std::mutex();
    run_cv = new std::condition_variable();
    finished_cv = new std::condition_variable();

    num_threads_ = num_threads;
    killed = false;
    pool = new std::thread[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::runthread, this);
    }
    

}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    killed = true;
    run_cv->notify_all();
    for (int i = 0; i < num_threads_; i++)
    {
        pool[i].join();
    }
    
    delete run_mutex;
    delete run_cv;
    delete finished_cv;
    delete[] pool;
}

void TaskSystemParallelThreadPoolSleeping::runthread() {
    while (!killed) {

        std::unique_lock<std::mutex> run_lock(*run_mutex);
        while (waiting_task.empty() && ready_task.empty() && !killed) {
            run_cv->wait(run_lock);
        }
        // run_lock.unlock();

        // run_mutex->lock();
        if (ready_task.empty()) {
            run_lock.unlock();
            continue;
        } else {
            TaskID task_id = ready_task.front();
            Task* todo_task = task_id_to_task[task_id];
            int todo = todo_task->to_do_num;
            int total = todo_task->total_num;
            if (++todo_task->to_do_num == total) {
                running_task++;
                ready_task.pop_front();
            }
            run_lock.unlock();
            
            todo_task->runnable_->runTask(todo, total);
            
            run_mutex->lock();
            if (++todo_task->finished_num == total) {
                finished_task.insert(task_id);
                running_task--;
                delete todo_task;
                // waiting to ready
                for (auto it = waiting_task.begin(); it != waiting_task.end(); ) {
                    Task* task = task_id_to_task[*it];
                    task->left_deps.erase(task_id);
                    if (task->left_deps.empty()) {
                        ready_task.push_back(*it);
                        it = waiting_task.erase(it);
                    } else {
                        it++;
                    }
                }
                // notify ()
                // run_cv->notify_all();
                if (waiting_task.empty() && ready_task.empty() && running_task==0) {
                    finished_cv->notify_one();
                }
            }
            run_mutex->unlock();

        }

    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    std::vector<TaskID> nodeps;
    runAsyncWithDeps(runnable, num_total_tasks, nodeps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    task_id_to_task[current_id] = new Task(current_id, num_total_tasks, runnable, deps);

    run_mutex->lock();
    for (auto& dep : deps) {
        if (finished_task.find(dep) != finished_task.end()) {
            task_id_to_task[current_id]->left_deps.erase(dep);
        }
    }

    if (task_id_to_task[current_id]->left_deps.empty()) {
        ready_task.push_back(current_id);
    } else {
        waiting_task.push_back(current_id);
    }
    run_mutex->unlock();
    run_cv->notify_all();

    return current_id++;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    std::unique_lock<std::mutex> run_lock(*run_mutex);
    while (!waiting_task.empty() || !ready_task.empty() || running_task!=0) {
        finished_cv->wait(run_lock);
    }
    return;
}
