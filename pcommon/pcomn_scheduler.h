/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SCHEDULER_H
#define __PCOMN_SCHEDULER_H
/*******************************************************************************
 FILE         :   pcomn_scheduler.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Trivial task scheduler

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Jun 2009
*******************************************************************************/
/** Trivial task scheduler
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_syncqueue.h>
#include <pcomn_syncobj.h>
#include <pcomn_atomic.h>
#include <pcomn_handle.h>

#include <map>
#include <set>
#include <list>
#include <functional>

namespace pcomn {

/*******************************************************************************
 Forward declarations
*******************************************************************************/
class Scheduler ;
class AsyncScheduler ;
typedef Scheduler SyncScheduler ;

/******************************************************************************/
/** Trivial synchronous scheduler: all tasks are running in the same thread.

 @note All time intervals are specified in microseconds.
*******************************************************************************/
class _PCOMNEXP Scheduler {
   public:
      typedef uint64_t taskid_t ;

      struct taskinfo {
            friend class Scheduler ;

            taskinfo() { memset(this, 0, sizeof *this) ; }

            taskid_t id() const { return _id ; }
            explicit operator bool() const { return !!id() ; }

            uint64_t sched_count() const { return _sched_count ; }
            int64_t last_expired() const { return _last_expired ; }
            int64_t left_to_expire() const { return _left_to_expire ; }
         private:
            taskid_t _id ;
            uint64_t _sched_count ;
            int64_t  _last_expired ;
            int64_t  _left_to_expire ;
      } ;

      /// Create a scheduler
      /// @param  init_id           The initial task ID: actually, the first id returned
      /// by schedule_task() will be init_id+1
      /// @param  schedthrd_stacksz  The default stacksize (in bytes) for the scheduler
      /// thread; since in a synchronous scheduler scheduled task(s) a running in the
      /// scheduler thread, this is also the stack size for scheduled task(s);
      /// 0 means to use the process/OS default value (which is, as a rule,
      /// @em way too big - e.g., 8MB on Linux).
      ///
      ///
      explicit Scheduler(taskid_t init_id = 0, size_t schedthrd_stacksz = 128*1024) ;

      virtual ~Scheduler() ;

      /// @param  task_fn           The task to run
      /// @param  init_expiration   The time before initial firing (usec); if 0, ignored
      /// (only @a repeat_interval is considered)
      /// @param  repeat_interval   Repeat interval (usec); if 0, there is no periodic scheduling
      /// @param  flags             Reserved, must be 0
      ///
      /// The @a task first runs after @a init_expiration microseconds and then repeats
      /// every @a repeat_interval microseconds. @a init_expiration specifies a
      /// one-shot timer, whereas @a repeat_interval a periodic timer. Task can be
      /// scheduled using a combination of one-shot and periodic behaviour.
      taskid_t schedule(const std::function<void()> &task_fn,
                        int64_t init_expiration, int64_t repeat_interval,
                        flags32_t flags = 0)
      {
         return
            schedule_task(make_job(task_fn), init_expiration, repeat_interval, flags, 0) ;
      }

      /// Reschedule the task
      ///
      /// @param  id                An identifier of the task to rechedule
      /// @param  init_expiration   The time before initial firing
      /// @param  repeat_interval   Repeat interval (usec); if 0, there is no periodic scheduling
      /// @note If @a init_expiration or/and @a repeat_interval <0, the respective
      /// scheduling parameters are not changed.
      ///
      std::pair<int64_t, int64_t> reschedule(taskid_t id, int64_t init_expiration, int64_t repeat_interval) ;

      /// Cancel task scheduling
      ///
      /// @param id     ID of a scheduled task; may be 0 or invalid task ID, it's
      /// perfectly OK
      /// @param wait_for_completion require synchronous cancel (i.e., if the task is
      /// currently running, the call to cancel will wait until it is finished)
      /// @note This method doesn't cancel currently running instance of @a id.
      taskinfo cancel(taskid_t id, bool wait_for_completion) ;

      taskinfo info(taskid_t id) const ;

      friend _PCOMNEXP std::ostream &operator<<(std::ostream &os, const Scheduler &s) ;

   protected:
      class ScheduledTask ;
      friend class ScheduledTask ;
      typedef shared_intrusive_ptr<ScheduledTask> task_ptr ;

      taskid_t schedule_task(const TaskPtr &task,
                             int64_t init_expiration, int64_t repeat_interval,
                             flags32_t flags, size_t stacksize) ;

      virtual task_ptr create_task(const TaskPtr &task, size_t stacksize) ;

      /*************************************************************************
                           class Scheduler::ScheduledTask
      *************************************************************************/
      class _PCOMNEXP ScheduledTask : public PRefCount {
            friend class Scheduler ;
         public:
            virtual void launch() = 0 ;
            virtual void force_finish(bool wait) = 0 ;
            virtual bool is_finish_forced() const = 0 ;

            Scheduler &owner() const { return _owner ; }
            const TaskPtr &task() { return _task ; }

            taskid_t id() const { return _id ; }

            std::pair<int64_t, uint64_t> event_tag() const
            {
               return std::pair<int64_t, uint64_t>(_next_sched, _event_id) ;
            }

            virtual std::ostream &debug_print(std::ostream &os) const ;

            friend std::ostream &operator<<(std::ostream &os, const ScheduledTask &v)
            {
               return v.debug_print(os) ;
            }

         protected:
            // Constructor automatically allocates task ID
            ScheduledTask(Scheduler &owner, const TaskPtr &task) ;

            bool synchronous_launch() ;
            void place_to_finished_queue() ;

         private:
            Scheduler &    _owner ; /* The employer */
            const TaskPtr  _task ;  /* A task we have got (to accomplish) */
            const taskid_t _id ;

            int64_t        _sched_interval ; /* Interval for periodic scheduling (0 for one-shot)*/
            int64_t        _next_sched ;     /* Absolute time point for the next run */
            uint64_t       _event_id ;

            uint64_t       _sched_count ;    /* The count of times thread was scheduled (informational) */
            uint64_t       _finished_count ;
            int64_t        _last_started ;   /* Time point of the last run (informational) */
            int64_t        _last_finished ;  /* Time point the last run has finished */
      } ;

   private:
      class _PCOMNEXP SyncTask : public ScheduledTask {
            typedef ScheduledTask ancestor ;
         public:
            SyncTask(Scheduler &owner, const TaskPtr &task) :
               ancestor(owner, task),
               _finish(false)
            {}

            void launch() ;
            void force_finish(bool wait) ;
            bool is_finish_forced() const { return _finish ; }

         private:
            bool              _finish ;
            std::recursive_mutex   _run_lock ;
      } ;

      // The functor to order scheduled tasks by their event tag.
      // Event tag both makes tasks with equal _next_sched time unique and provides
      // stable ordering (from the two tasks with equal scheduling times the later added
      // has greater _event_id)
      struct order_events : std::binary_function<task_ptr, task_ptr, bool> {
            bool operator()(const task_ptr &left, const task_ptr &right) const
            {
               NOXCHECK(left && right) ;
               return left->event_tag() < right->event_tag() ;
            }
      } ;

      typedef std::map<taskid_t, task_ptr>      task_map ;
      typedef std::set<task_ptr, order_events>  task_queue ;
      typedef std::list<task_ptr>               task_list ;
      typedef std::pair<fd_safehandle, fd_safehandle> event_port ;
      typedef std::unique_lock<std::recursive_mutex> sched_lock ;

   private:
      mutable std::recursive_mutex _mutex ;

      taskid_t    _last_allocated_id ; /* The last allocated task ID */
      uint64_t    _event_counter ;     /* Monotonically increasing scheduling
                                              * event counter */
      event_port  _event_port ;

      task_map    _task_map ;
      task_queue  _task_queue ;
      task_list   _finished_queue ;   /* The queue of both the already finished
                                       * and having been finished tasks */
      TaskThread  _sched_thread ; /* The main scheduler thread */

      enum Event {
         EV_RESCHEDULE  = 'R',
         EV_FINISHED    = 'F',   /* One of the tasks just completely finished and should
                                  * be removed from the scheduler */
         EV_STOP        = 'S'    /* Stop the scheduler completely */
      } ;

      taskid_t allocate_taskid() { return atomic_op::inc(&_last_allocated_id) ; }

      task_queue::iterator queue_task(const task_ptr &task) ;
      void queue_and_reschedule(const task_ptr &task) ;

      bool is_destroying() const { return _event_port.second.bad() ; }

      // Check tasks in the queue, launch already expired and return the timepoint of the
      // next rescheduling
      int64_t reschedule_tasks() ;
      void handle_finished_queue() ;
      void stop_scheduler() ;

      void create_event_port() ;
      void post_event(Event) ;
      Event get_event(int64_t usec_timeout) ;

      // The main scheduler event loop, runs inside _sched_thread
      void handle_events() ;
      // Called once from the start of handle_events
      void init_eventhandler() ;
} ;

/******************************************************************************/
/** Asynchronous multithreaded scheduler: allocates a dedicated thread for every task
*******************************************************************************/
class _PCOMNEXP AsyncScheduler : public Scheduler {
      typedef Scheduler ancestor ;
   public:
      /// Create a scheduler
      /// @param  init_id           The initial task ID: actually, the first id returned
      /// by schedule_task() will be init_id+1
      /// @param  worker_stacksize  The default stacksize (in bytes) for worker threads
      /// allocated to run scheduled tasks: 0 means to use the process/OS default value;
      /// note that stack size can later be specified on per-task basis in
      /// schedule_task() call
      ///
      explicit AsyncScheduler(taskid_t init_id = 0, size_t worker_stacksize = 0) ;

      /// @param  task              The task to run
      /// @param  init_expiration   The time before initial firing
      /// @param  repeat_interval   Repeat interval (usec); if 0, there is no periodic scheduling
      /// @param  flags             Reserved, must be 0
      /// @param  worker_stacksize  Worker thread stack size
      taskid_t schedule(const TaskPtr &task,
                        int64_t init_expiration, int64_t repeat_interval,
                        flags32_t flags = 0, size_t worker_stacksize = 0)
      {
         return
            schedule_task(task, init_expiration, repeat_interval, flags, worker_stacksize) ;
      }

      /// @overload
      taskid_t schedule(const std::function<void()> &task_fn,
                        int64_t init_expiration, int64_t repeat_interval,
                        flags32_t flags = 0, size_t worker_stacksize = 0)
      {
         return
            schedule(make_job(task_fn), init_expiration, repeat_interval, flags, worker_stacksize) ;
      }

   private:
      size_t _worker_stack_size ; /* Default worker thread stack size */

      task_ptr create_task(const TaskPtr &task, size_t stacksize) ;

      /*************************************************************************
                           class AsyncScheduler::AsyncTask
      *************************************************************************/
      class _PCOMNEXP AsyncTask : public BasicThread, public ScheduledTask {
            friend class AsyncScheduler ;

            typedef BasicThread ancestor ;
         public:
            // Constructor automatically allocates task ID
            AsyncTask(AsyncScheduler &owner, const TaskPtr &task, size_t stacksize) ;
            ~AsyncTask() ;

            void launch() ;
            // Close the _sched_queue -> force thread to finish
            void force_finish(bool wait) ;
            bool is_finish_forced() const ;

            std::ostream &debug_print(std::ostream &os) const ;

            friend std::ostream &operator<<(std::ostream &os, const AsyncTask &v)
            {
               return v.debug_print(os) ;
            }
         private:
            mutable ProducerConsumerLock _sched_queue ;

            // Overrides BasicThread::run.
            int run() ;

            bool is_queued_for_completion() const { return !!event_tag().second ; }
      } ;
      friend class AsyncTask ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_SCHEDULER_H */
