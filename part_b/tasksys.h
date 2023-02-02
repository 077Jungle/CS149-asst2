#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

#include <mutex>
#include <condition_variable>

#include <deque>
#include <map>
#include <set>
#include <thread>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */

class Task {
    public:
        TaskID task_id;

        int to_do_num;
        int finished_num;
        int total_num;

        IRunnable* runnable_;

        std::set<TaskID> left_deps;

        Task(TaskID task_id, int num_total_tasks, IRunnable* runnable, const std::vector<TaskID>& deps): task_id(task_id), 
                                                                                                        to_do_num(0), 
                                                                                                        finished_num(0), 
                                                                                                        total_num(num_total_tasks), 
                                                                                                        runnable_(runnable), 
                                                                                                        left_deps(deps.begin(), deps.end()) {}
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        int current_id;
        std::map<TaskID, Task*> task_id_to_task;
        std::vector<TaskID> waiting_task;
        std::deque<TaskID> ready_task;
        int running_task;
        std::set<TaskID> finished_task;

        std::mutex* run_mutex;
        std::condition_variable* run_cv;
        std::condition_variable* finished_cv;

        int num_threads_;
        std::thread* pool;
        void runthread();
        bool killed;
};

#endif
