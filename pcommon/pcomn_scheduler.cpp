/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_scheduler.cpp
 COPYRIGHT    :   Yakov Markovitch, 2010-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Trivial task scheduler

 CREATION DATE:   8 Jun 2009
*******************************************************************************/
#include <pcomn_scheduler.h>
#include <pcomn_timespec.h>
#include <pcomn_diag.h>

#include <functional>

#include <signal.h>
#include <poll.h>
#include <limits.h>

namespace pcomn {

static const size_t  SCHEDTHRD_MINSTACKSZ = 64*1024 ;
static const int64_t INTRVAL_RES = 1000 ; /* Interval resolution, useconds */

/*******************************************************************************
 Scheduler
*******************************************************************************/
Scheduler::Scheduler(taskid_t init_id, size_t shedthrd_stacksz) :
   _last_allocated_id(init_id),
   _event_counter(0),

   _sched_thread(make_job(std::bind(std::mem_fn(&Scheduler::handle_events), this)),
                 BasicThread::JoinAuto,
                 pcomn::inrange(shedthrd_stacksz, (size_t)1, SCHEDTHRD_MINSTACKSZ)
                 ? SCHEDTHRD_MINSTACKSZ : shedthrd_stacksz)
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, "Creating " << diag::otptr(this)) ;

   create_event_port() ;

   TRACEPX(PCOMN_Scheduler, DBGL_HIGHLEV, "Starting task thread of " << diag::otptr(this)) ;

   _sched_thread.start(BasicThread::StartRunning) ;
}

Scheduler::~Scheduler()
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, "Deleting " << *this) ;

   // Send message to stop the task thread
   post_event(EV_STOP) ;

   // Now the destructor of _sched_thread will force to wait until _sched_thread
   // finished (joined), then destructors of _task_map and _task_queue will wait until
   // worker threads finished
   TRACEPX(PCOMN_Scheduler, DBGL_HIGHLEV, "Waiting for threads of " << diag::otptr(this) << " to stop") ;
}

inline Scheduler::task_queue::iterator Scheduler::queue_task(const task_ptr &task)
{
   TRACEPX(PCOMN_Scheduler, DBGL_VERBOSE, "Queue task " << *task << " for scheduling") ;

   NOXCHECK(task->_next_sched > 0) ;
   task->_event_id = ++_event_counter ;
   return _task_queue.insert(task).first ;
}

inline void Scheduler::queue_and_reschedule(const task_ptr &task)
{
   if (task->_next_sched)
   {
      task_queue::iterator queued (queue_task(task)) ;
      if (queued == _task_queue.begin())
         post_event(EV_RESCHEDULE) ;
   }
}

Scheduler::task_ptr Scheduler::create_task(const TaskPtr &task, size_t)
{
   return task_ptr(new SyncTask(*this, task)) ;
}

Scheduler::taskid_t
Scheduler::schedule_task(const TaskPtr &taskptr, int64_t init_expiration, int64_t repeat_interval,
                         flags32_t flags, size_t stacksize)
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, SCOPEMEMFNOUT(taskptr << init_expiration << repeat_interval
                                                       << flags << stacksize) << ", this=" << *this) ;
   PCOMN_USE(flags) ;

   PCOMN_ENSURE_ARG(taskptr) ;

   const task_ptr &task = create_task(taskptr, stacksize) ;

   task->_sched_interval = repeat_interval > 0 ? std::max(repeat_interval, INTRVAL_RES) : 0 ;
   init_expiration = std::max<int64_t>(init_expiration, 0) ;

   if (init_expiration)
      task->_next_sched = time_point::now().as_useconds() + init_expiration ;
   else if (task->_sched_interval)
      task->_next_sched = time_point::now().as_useconds() + task->_sched_interval ;

   sched_lock lock(_mutex) ;

   _task_map.insert(task_map::value_type(task->id(), task)) ;
   queue_and_reschedule(task) ;

   return task->id() ;
}

std::pair<int64_t, int64_t>
Scheduler::reschedule(taskid_t id, int64_t init_expiration, int64_t repeat_interval)
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, SCOPEMEMFNOUT(id << init_expiration << repeat_interval)
           << ", this=" << *this) ;

   typedef std::pair<int64_t, int64_t> result_type ;

   sched_lock lock(_mutex) ;

   const task_map::iterator found (_task_map.find(id)) ;
   if (found == _task_map.end())
   {
      TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Task " << id << " not found in " << *this) ;
      return result_type(-1, -1) ;
   }

   const task_ptr task (found->second) ;

   const int64_t new_interval = repeat_interval < 0
      ? task->_sched_interval
      : (repeat_interval ? std::max(repeat_interval, INTRVAL_RES) : 0) ;

   const int64_t new_sched = init_expiration < 0
      ? task->_next_sched
      : (init_expiration > 0
         ? time_point::now().as_useconds() + init_expiration
         : (task->_sched_interval > 0
            ? time_point::now().as_useconds() + task->_sched_interval
            : 0)) ;

   const result_type result (task->_next_sched, task->_sched_interval) ;

   if (result != result_type(new_sched, new_interval))
   {
      NOXCHECK(new_sched > 0 || !new_sched && !new_interval) ;

      _task_queue.erase(task) ;
      task->_next_sched = new_sched ;
      task->_sched_interval = new_interval ;

      queue_and_reschedule(task) ;
   }

   return result ;
}

Scheduler::taskinfo Scheduler::cancel(taskid_t id, bool wait)
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, SCOPEMEMFNOUT(id << wait) << ", this=" << *this) ;

   taskinfo result ;
   sched_lock lock(_mutex) ;

   task_map::iterator found (_task_map.find(id)) ;
   if (found == _task_map.end())
   {
      TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, "Scheduled task " << id << " not found") ;
      return result ;
   }

   const task_ptr task (found->second) ;

   // Remove the task from the taskmap (it is definitely there)
   _task_map.erase(found) ;
   // Try to remove the task from the queue (it _may_ be there)
   _task_queue.erase(task) ;

   if (wait)
      task->_event_id = 0 ;

   // Release the mutex: all following we can do out of the critical section
   lock.unlock() ;

   // Force finishing the task (and, possibly, wait for completion)
   task->force_finish(wait) ;

   result._id = id ;
   return result ;
}

void Scheduler::handle_events()
{
   init_eventhandler() ;

   TRACEPX(PCOMN_Scheduler, DBGL_HIGHLEV, "Start handling events by " << *this) ;

   try {
      int64_t timeout = -1 ;
      for (Event ev ; (ev = get_event(timeout)) != EV_STOP ;)
      {
         int64_t next_schedule ;
         switch (ev)
         {
            case EV_FINISHED:
               handle_finished_queue() ;
               // Note there is _deliberately_ no 'break' here: we must always reschedule
               // after handling finished tasks

            case EV_RESCHEDULE:
               // reschedule_tasks() will return -1 if _task_queue became empty after
               // rescheduling
               next_schedule = reschedule_tasks() ;
               timeout = next_schedule < 0 ?
                  -1 : std::max<int64_t>(next_schedule - time_point::now().as_useconds(), 0) ;
               break ;

            default: NOXFAIL("Unknown scheduler event") ;
         }
      }

      stop_scheduler() ;
   }
   catch (const std::exception &x) {
      // If we've got an exception in the scheduler event thread, there is nothing for it
      // but to abort a process (we have no way to inform the rest of our program the
      // sheduler is broken).
      LOGPXALERT_CALL(NOXFAIL, PCOMN_Scheduler,
                      STDEXCEPTOUT(x) << " handling events for scheduler " << *this) ;
   }
}

int64_t Scheduler::reschedule_tasks()
{
   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Reschedule tasks " << *this) ;

   const time_point &now = time_point::now() ;

   sched_lock lock(_mutex) ;
   for (task_queue::iterator i = _task_queue.begin() ; i != _task_queue.end() ; i = _task_queue.begin())
   {
      task_ptr task (*i) ;
      if (task->_next_sched >= now.as_useconds() + INTRVAL_RES)
         return task->_next_sched ;

      _task_queue.erase(i) ;

      if (task->_sched_interval)
      {
         // Periodic scheduling, re-queue the task
         task->_next_sched += task->_sched_interval ;
         if (task->_next_sched < now.as_useconds() + INTRVAL_RES)
            task->_next_sched = now.as_useconds() + task->_sched_interval ;
         queue_task(task) ;
      }
      else
         task->_next_sched = 0 ;

      // Did we have enough time to complete running before the next scheduling?
      // If not, skip this round
      if (task->_sched_count == atomic_op::get(&task->_finished_count))
      {
         TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Schedule the task " << *task << " to run now ("
                 << now << "), " << *this) ;

         task->_last_started = now.as_useconds() ;
         ++task->_sched_count ;

         // Temporary release the mutex
         lock.unlock() ;

         // Launch the task
         task->launch() ;

         // Reacquire mutex
         lock.lock() ;
      }
      else
         TRACEPX(PCOMN_Scheduler, DBGL_NORMAL,
                 "Previous run of the task " << *task << " is not yet finished, skip scheduling") ;
   }

   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Rescheduled tasks for " << *this) ;

   return _task_queue.empty() ? -1 : (*_task_queue.begin())->_next_sched ;
}

void Scheduler::handle_finished_queue()
{
   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Handle finished queue " << *this) ;

   task_list completed ;

   sched_lock lock(_mutex) ;
   // Finished task worker threads are only referenced from _finished_queue. Since
   // thread destructor joins automatically, it is enough to clear _finished_queue
   // items to ensure finished threads are actually finished.
   pcomn_swap(completed, _finished_queue) ;
   lock.unlock() ;

   // Now destructor of 'completed' will do the work
}

void Scheduler::stop_scheduler()
{
   TRACEPX(PCOMN_Scheduler, DBGL_ALWAYS, "Stop scheduler " << *this) ;

   sched_lock lock(_mutex) ;

   for (task_map::iterator i = _task_map.begin(), e = _task_map.end() ; i != e ; ++i)
      i->second->force_finish(false) ;
}

void Scheduler::create_event_port()
{
   TRACEPX(PCOMN_Scheduler, DBGL_HIGHLEV, "Creating event port for " << diag::otptr(this)) ;

   int pipe_ends[2] ;
   PCOMN_ENSURE_POSIX(pipe(pipe_ends), "pipe") ;
   _event_port.first.reset(pipe_ends[0]) ;
   _event_port.second.reset(pipe_ends[1]) ;
}

void Scheduler::post_event(Event ev)
{
   const char event = ev ;
   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Event '" << event << "' to " << *this) ;

   NOXCHECK(ev == EV_RESCHEDULE || ev == EV_FINISHED || ev == EV_STOP) ;

   sched_lock lock(_mutex) ;

   if (_event_port.second.bad())
      // Already stopped
      return ;

   if (ev == EV_STOP)
      // Close the writing end of the pipe on EV_STOP to force poll() return POLLHUP
      // event
      _event_port.second.reset() ;
   else
      NOXVERIFY(::write(_event_port.second, &event, 1) == 1) ;
}

Scheduler::Event Scheduler::get_event(int64_t usec_timeout)
{
   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Get event with timeout " << usec_timeout
           << " usec for " << *this) ;

   pollfd ev ;
   ev.fd = _event_port.first ;
   ev.events = POLLIN ;
   ev.revents = 0 ;

   const int msec_timeout =
      usec_timeout < 0 ? -1 : (int)std::min<int64_t>((usec_timeout + (INTRVAL_RES - 1))/INTRVAL_RES, INT_MAX) ;

   switch (::poll(&ev, 1, msec_timeout))
   {
      case 1:
      {
         TRACEPX(PCOMN_Scheduler, DBGL_VERBOSE, "Got new event " << ev.revents << " in " << *this) ;
         if (ev.revents != POLLIN)
            return EV_STOP ;

         char result = 0 ;
         PCOMN_ENSURE_POSIX(::read(_event_port.first, &result, 1), "read") ;
         PCOMN_VERIFY(result == EV_RESCHEDULE || result == EV_FINISHED) ;
         return (Event)result ;
      }

      case 0:
         // poll() returned 0, timeout expired
         TRACEPX(PCOMN_Scheduler, DBGL_VERBOSE, "Event timeout expired for " << *this) ;
         break ;

      case -1:
         // If poll() is interrupted by a signal, it is safe to return EV_RESCHEDULE to
         // make a scheduler FSA to reinit itself
         if (errno != EINTR)
            PCOMN_THROW_SYSERROR("poll") ;
   }

   return EV_RESCHEDULE ;
}

void Scheduler::init_eventhandler()
{
   // Mask all signal for the handler threads (except for SIGBUS, SIGFPE, SIGILL,
   // SIGSEGV: if those signals generated while they are blocked, the result is
   // undefined)
   sigset_t every_signal ;
   sigfillset(&every_signal) ;
   sigdelset(&every_signal, SIGBUS) ;
   sigdelset(&every_signal, SIGFPE) ;
   sigdelset(&every_signal, SIGILL) ;
   sigdelset(&every_signal, SIGSEGV) ;

   pthread_sigmask(SIG_BLOCK, &every_signal, NULL) ;
}

std::ostream &operator<<(std::ostream &os, const Scheduler &s)
{
   Scheduler::sched_lock lock(s._mutex) ;

   return os
      << '<' << diag::otptr(&s) << " tasks:" << s._task_map.size() << " queued:" << s._task_queue.size()
      << " lastid:" << s._last_allocated_id << " evcnt:" << s._event_counter << '>' ;
}

/*******************************************************************************
 AsyncScheduler
*******************************************************************************/
AsyncScheduler::AsyncScheduler(taskid_t init_id, size_t worker_stacksize) :
   ancestor(init_id, SCHEDTHRD_MINSTACKSZ),
   _worker_stack_size(worker_stacksize)
{}

Scheduler::task_ptr AsyncScheduler::create_task(const TaskPtr &task, size_t stacksize)
{
   return task_ptr(new AsyncTask(*this, task, stacksize)) ;
}

/*******************************************************************************
 Scheduler::ScheduledTask
*******************************************************************************/
Scheduler::ScheduledTask::ScheduledTask(Scheduler &owner, const TaskPtr &task) :
   _owner(owner),
   _task(PCOMN_ENSURE_ARG(task)),
   _id(owner.allocate_taskid()),
   _sched_interval(0),
   _next_sched(0),
   _event_id(0),

   _sched_count(0),
   _finished_count(0),

   _last_started(0),
   _last_finished(0)
{}

bool Scheduler::ScheduledTask::synchronous_launch()
{
   TRACEPX(PCOMN_Scheduler, DBGL_LOWLEV, "Launched " << *this) ;

   const bool finish_forced = is_finish_forced() ;

   if (!finish_forced)
      try {
         _task->accomplish() ;

         TRACEPX(PCOMN_Scheduler, DBGL_LOWLEV, "Normally finished " << *this) ;
      }
      catch (const std::exception &x)
      {
         LOGPXWARN(PCOMN_Scheduler, "Exception in scheduled task: " << STDEXCEPTOUT(x)) ;
      }
   else
      TRACEPX(PCOMN_Scheduler, DBGL_LOWLEV, "Finish forced for " << *this) ;

   atomic_op::xchg(&_last_finished, time_point::now().as_useconds()) ;
   NOXCHECK(finish_forced || _finished_count < _sched_count) ;
   atomic_op::xchg(&_finished_count, _sched_count) ;

   return !finish_forced ;
}

void Scheduler::ScheduledTask::place_to_finished_queue()
{
   sched_lock lock(owner()._mutex) ;
   if (!owner().is_destroying())
   {
      // Don't assign this worker to pending completion if called from the
      // Scheduler's destructor
      owner()._finished_queue.push_back(task_ptr(this)) ;
      owner().post_event(EV_FINISHED) ;
   }
}

std::ostream &Scheduler::ScheduledTask::debug_print(std::ostream &os) const
{
   return os << "task=" << _id << " nxt=" << time_point(_next_sched)
             << " intrvl=" << _sched_interval
             << " cnt=(" << _sched_count << ", " << _finished_count << ')' ;
}

/*******************************************************************************
 Scheduler::SyncTask
*******************************************************************************/
void Scheduler::SyncTask::launch()
{
   PCOMN_SCOPE_LOCK (run_lock_guard, _run_lock) ;

   synchronous_launch() ;
}

void Scheduler::SyncTask::force_finish(bool wait)
{
   _finish = true ;
   if (wait)
   {
      _run_lock.lock() ;
      _run_lock.unlock() ;
   }
}

/*******************************************************************************
 AsyncScheduler::AsyncTask
*******************************************************************************/
AsyncScheduler::AsyncTask::AsyncTask(AsyncScheduler &owner, const TaskPtr &task, size_t stacksize) :
   ancestor(JoinAuto, stacksize ? stacksize : owner._worker_stack_size),
   ScheduledTask(owner, task)
{
   start(StartRunning) ;
}

AsyncScheduler::AsyncTask::~AsyncTask()
{
   _sched_queue.close() ;
   destroy() ;
}

bool AsyncScheduler::AsyncTask::is_finish_forced() const
{
   try {
      ConsumerGuard(_sched_queue).consume() ;
   }
   catch (const object_closed &)
   {
      if (is_queued_for_completion())
         const_cast<AsyncTask *>(this)->place_to_finished_queue() ;
      return true ;
   }
   return false ;
}

void AsyncScheduler::AsyncTask::launch()
{
   TRACEPX(PCOMN_Scheduler, DBGL_VERBOSE, SCOPEMEMFNOUT(diag::endargs)) ;
   try {
      ProducerGuard(_sched_queue).produce() ;
   }
   catch (const object_closed &)
   {
      WARNPX(PCOMN_Scheduler, true, DBGL_ALWAYS, "Attempt to launch already finsihed " << *this) ;
   }
}

// Never _EVER_ call this from the async task thread itself
void AsyncScheduler::AsyncTask::force_finish(bool wait)
{
   _sched_queue.close() ;
   if (wait)
      join() ;
}

int AsyncScheduler::AsyncTask::run()
{
   TRACEPX(PCOMN_Scheduler, DBGL_NORMAL, "Started sheduler thread " << *this
           << " of " << owner()) ;

   // The main procesing cycle
   while (synchronous_launch()) ;

   return 1 ;
}

std::ostream &AsyncScheduler::AsyncTask::debug_print(std::ostream &os) const
{
   return ScheduledTask::debug_print(ancestor::debug_print(os) << "; ") ;
}

} // end of namespace pcomn
