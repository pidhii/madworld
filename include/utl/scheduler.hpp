#ifndef UTL_SCHEDULER_HPP
#define UTL_SCHEDULER_HPP


#include "utl/safe_access.hpp"

#include <mutex>
#include <queue>


namespace mw {


struct task: safe_access<task> {
  virtual
  void operator () () = 0;
};


/**
 * @brief One-time-task handle
 * @details Executes `thunk` one time and deletes itself.
 * @see Use one_time_task() as a convenient way for creation of this kind of task.
 */
template <typename Thunk>
struct one_time_task_type: public task {
  one_time_task_type(const Thunk &thunk): thunk {thunk} { }

  void
  operator () () override
  { thunk(); delete this; }

  Thunk thunk;
};

template <typename Thunk> task*
one_time_task(const Thunk &thunk)
{ return new one_time_task_type<Thunk> {thunk}; }


/**
 * @brief Dummy *BasicLockable* object
 * @details To be used as a placeholder lockable object without performance penalties
 * @see [*BasicLockable*](https://en.cppreference.com/w/cpp/named_req/BasicLockable.html)
 */
struct dummy_lock { void lock() { } void unlock() noexcept { } };

/**
 * @brief Time-based task scheduler
 */
template <typename Clock, typename Lock = dummy_lock>
struct scheduler {
  using clock = Clock;
  using time_point = typename clock::time_point;
  using duration = typename clock::duration;

  scheduler(Lock &lock): lock {lock} { }
  scheduler(): lock {_dummy} { }

  void
  add_task(const time_point &time, task *task)
  { std::lock_guard _ {lock}; tasks.emplace(time, task); }

  void
  add_task(task *task)
  { add_task(clock::now(), task); }

  template <typename Duration> void
  add_task(const Duration &dt, task *task)
  {
    const duration dt_ = std::chrono::duration_cast<duration>(dt);
    add_task(clock::now() + dt_, task);
  }

  using task_entry_type = std::pair<time_point, safe_pointer<task>>;
  struct _compare_tasks {
    bool
    operator () (const task_entry_type &a, const task_entry_type &b) const
    { return a.first > b.first; }
  };

  Lock &lock;
  std::priority_queue<task_entry_type, std::vector<task_entry_type>, _compare_tasks>
    tasks;
  dummy_lock _dummy;
};


template <typename Clock, typename Lock> void
run_single_task(scheduler<Clock, Lock> &sched)
{
  while (not sched.tasks.empty())
  {
    std::lock_guard _ {sched.lock};

    // Pick first task that is alive
    const auto [time, task] = sched.tasks.top();
    if (not task)
    { // Task is dead; remove it and try again
      sched.tasks.pop();
      continue;
    }

    // Execute the task if the time has come and drop it; otherwize, return
    // without doing anything
    if (time <= Clock::now())
    {
      sched.tasks.pop();
      (*task)();
    }
    return;
  }
}

} // namespace mw

#endif