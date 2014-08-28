/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_JOBQUEUE_H
#define __PCOMN_JOBQUEUE_H
/*******************************************************************************
 FILE         :   pcomn_jobqueue.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Job queue.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Jul 2001
*******************************************************************************/
#include <pcomn_syncqueue.h>
#include <pcomn_thread.h>
#include <pcomn_omanip.h>

#include <queue>
#include <functional>

namespace pcomn {
/******************************************************************************/
/** Asynchronous job queue.

 Jobs can be submitted into the queue asynchronously. The submitted job are retrieved
 from the queue and processed one-by-one.

  @param Job      The job class: objects of this class are put into the queue.
  @param Handler  A functor class (callable object). The object of this class
  is called with a Job object as a parameter to process a job.
*******************************************************************************/
template<class Job, class Handler = std::function<void(const Job &)> >
class JobQueue {
   public:
      typedef Job       job_type ;
      typedef Handler   handler_type ;

      /// Create a job queue of a given size.
      /// @param size      Queue length.
      /// @param handler   A callable object, i.e. functor object that will be called
      /// for each job.
      /// @param stack_size   The stack size of a conveyer thread; 0 means OS default value.
      JobQueue(size_t size, const Handler &handler, size_t stack_size = 0) :
         _job_queue(size),
         _handler(handler),
         _conveyer(*this, stack_size)
      {
         NOXCHECK(size > 0) ;
         _conveyer.resume() ;
      }

      explicit JobQueue(size_t size, size_t stack_size = 0) :
         _job_queue(size),
         _conveyer(*this, stack_size)
      {
         NOXCHECK(size > 0) ;
         _conveyer.resume() ;
      }

      ~JobQueue() { terminate() ; }

      void submit(const Job &job) { _job_queue.push(job) ; }

      void close()
      {
         _job_queue.close() ;
         if (_conveyer.handle())
            _conveyer.join() ;
      }

      void terminate()
      {
         _job_queue.terminate() ;
         if (_conveyer.handle())
            _conveyer.join() ;
      }

   private:
      class Conveyer ;
      friend class Conveyer ;

      class Conveyer : public BasicThread {
         public:
            Conveyer(JobQueue &queue, size_t stacksz) :
               BasicThread(JoinManually, stacksz),
               _job_queue(queue)
            {
               start(StartSuspended) ;
            }

         protected:
            int run()
            {
               try {
                  while(true)
                     _job_queue._handler(_job_queue._job_queue.pop()) ;
               }
               catch(const object_closed &)
               {}
               return 1 ;
            }

         private:
            JobQueue &_job_queue ;
      } ;

      pcomn::synchronized_queue<Job> _job_queue ;
      Handler                        _handler ;
      Conveyer                       _conveyer ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_JOBQUEUE_H */
