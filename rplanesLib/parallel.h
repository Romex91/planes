#pragma once
#include <omp.h>
class Mutex
{
public:
	Mutex()
	{
		omp_init_lock(&lock_);
	}
	~Mutex()
	{
		omp_destroy_lock(&lock_);
	}
	Mutex(const Mutex& ) { omp_init_lock(&lock_); }
	Mutex& operator= (const Mutex& ) { return *this; }
	friend class MutexLocker;
private:
	omp_lock_t lock_;
};

class MutexLocker
{
public:
	explicit MutexLocker( Mutex & Mutex ): lock_(Mutex.lock_)
	{
		omp_set_lock(&lock_);
	}
	~MutexLocker()
	{
		omp_unset_lock(&lock_);
	}
private:
	void operator=(const MutexLocker&);
	MutexLocker(const MutexLocker&);
	omp_lock_t & lock_;
};
class IdGetter
{
public:
	IdGetter(): idCounter_(0)
	{}
	size_t getID()
	{
		MutexLocker ml(mutex_);
		size_t retval;
		retval = idCounter_++;
		return retval;
	}
private:
	size_t idCounter_;
	Mutex mutex_;
};

