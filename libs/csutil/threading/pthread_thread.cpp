/*
  Copyright (C) 2006 by Marten Svanfeldt

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"

#include "csutil/threading/thread.h"
#include "csutil/threading/pthread_thread.h"
#include "csutil/threading/barrier.h"
#include "csutil/threading/condition.h"


namespace CS
{
namespace Threading
{
namespace Implementation
{

  namespace
  {

    static void* proxyFunc (void* param)
    {
      // Extract the parameters
      ThreadBase* tb = static_cast<ThreadBase*> (param);
      Runnable* runnable = tb->GetRunnable();

    #ifdef CS_HAVE_PTHREAD_SETNAME_NP
      {
        // Set the name, for debugging
        const char* threadName = runnable->GetName ();
        if (threadName)
          pthread_setname_np (pthread_self(), threadName);
      }
    #endif

      // Run
      runnable->Run ();

      return 0;
    }

  }


  ThreadBase::ThreadBase (Runnable* runnable)
    : runnable (runnable), isRunning (false), priority (THREAD_PRIO_NORMAL)
  {
  }

  ThreadBase::~ThreadBase ()
  {
    if (IsRunning ())
    {
      pthread_join (threadHandle, 0);
    }
  }

  void ThreadBase::Start ()
  {
    if (!IsRunning ())
    {
      int res;

      // The default is a joinable thread.
      res = pthread_create(&threadHandle, 0, proxyFunc, this);
      if (res == 0)
      {
        isRunning = true;

        // Set priority to make sure its updated if we set it before starting
        SetPriority (priority);
      }
    }
  }

  void ThreadBase::Stop ()
  {
    if (IsRunning ())
    {
      isRunning = false;
      pthread_cancel (threadHandle);
      pthread_join (threadHandle, 0);
    }
  }

  bool ThreadBase::SetPriority (ThreadPriority prio)
  {
    int res = 0;

    if (IsRunning ())
    {
      struct sched_param SchedulerProperties;

      // Clear the properties initially
      memset(&SchedulerProperties, 0, sizeof (struct sched_param));

      // Map the CS thread priority identifier to an appropriate platform specific identifier
      //  or fail if this mapping is not possible.
      switch(prio)
      {
      case THREAD_PRIO_LOW:
        // Posix Pthreads does not guarantee support for any compatible priority,
        //  so we'll default to NORMAL
      case THREAD_PRIO_NORMAL:
        SchedulerProperties.sched_priority = sched_get_priority_max (SCHED_OTHER);
        res = pthread_setschedparam (threadHandle, SCHED_OTHER, &SchedulerProperties);
        break;

      case THREAD_PRIO_HIGH:
        SchedulerProperties.sched_priority = sched_get_priority_max (SCHED_RR) - 1;
        res = pthread_setschedparam (threadHandle, SCHED_RR, &SchedulerProperties);
        break;
      }
    }

    if (res == 0)
    {
      priority = prio;
    }

    return res == 0;
  }

  void ThreadBase::Wait () const
  {
    if (IsRunning ())
    {
      isRunning = false;
      pthread_join (threadHandle, 0);
    }
  }

  void ThreadBase::Yield ()
  {
    sched_yield ();
  }

  ThreadID ThreadBase::GetThreadID ()
  {
    return (ThreadID)pthread_self();
  }
}
}
}
