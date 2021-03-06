/*
 * Mutex.h
 *
 *  Created on: Jun 26, 2014
 *      Author: damonhao
 */

#ifndef SIMPLEV_BASE_MUTEX_H_
#define SIMPLEV_BASE_MUTEX_H_

#include <assert.h>
#include <pthread.h>

#include <boost/noncopyable.hpp>

#include <simplev/base/CurrentThread.h>

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    __THROW __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else  // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

#endif // CHECK_PTHREAD_RETURN_VALUE


namespace simplev
{
class MutexLock: boost::noncopyable
{
public:
	MutexLock()
		:holder_()
	{
		MCHECK(pthread_mutex_init(&mutex_, NULL));
	}
	~MutexLock()
	{
		assert(holder_ == 0);
		MCHECK(pthread_mutex_destroy(&mutex_));
	}
	bool isLockedByThisThread()
	{
		return holder_ == CurrentThread::tid();
	}

	void assertLocked()
	{
		assert(isLockedByThisThread());
	}

	// internal usage
	void lock()
	{
		MCHECK(pthread_mutex_lock(&mutex_));
		assignHolder();
	}

	void unlock()
	{
		unassignHolder();
		MCHECK(pthread_mutex_unlock(&mutex_));
	}

  pthread_mutex_t* getPthreadMutex() // non-const
  {
    return &mutex_;
  }

private:
  friend class Condition;

  class UnassignGuard : boost::noncopyable
  {
   public:
    UnassignGuard(MutexLock& owner)
      : owner_(owner)
    {
      owner_.unassignHolder(); //UnassignGuard define in MutexLock, so it can call unassignHolder().
    }

    ~UnassignGuard()
    {
      owner_.assignHolder();
    }

   private:
    MutexLock& owner_;
  };

  void unassignHolder()
  {
    holder_ = 0;
  }

  void assignHolder()
  {
    holder_ = CurrentThread::tid();
  }
	pthread_mutex_t mutex_;
	pid_t holder_;
};

class MutexLockGuard : boost::noncopyable
{
public:
	explicit MutexLockGuard(MutexLock& mutex)
		:mutex_(mutex)
	{
		mutex_.lock();
	}

	~MutexLockGuard()
	{
		mutex_.unlock();
	}
private:
	MutexLock& mutex_; //note: just a referrence;
};
}

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif /* SIMPLEV_BASE_MUTEX_H_ */
