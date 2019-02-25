#include <sys/time.h>
#include "awe_mutex.h"
#include <stdio.h>

event::event(bool auto_reset, bool init) :
        _event_signaled(init),
        _event_auto_reset(auto_reset)
{
    pthread_mutex_init(&_event_mutex, NULL);
    pthread_cond_init(&_event_cond, NULL);
}

event::~event()
{
}

event *event::create_event(bool auto_reset, bool init)
{
    return new event(auto_reset, init);
}

int event::set_event()
{
    int ret = 0;
    // set event marked
    _event_signaled = true;
    pthread_cond_signal(&_event_cond);
    return ret;
}

int event::reset_event()
{
    int ret = 0;
    // remove event
    _event_signaled = false;
    event_leave();
    return ret;
}

void event::event_enter()
{
    pthread_mutex_lock(&_event_mutex);
}

void event::event_leave()
{
    pthread_mutex_unlock(&_event_mutex);
}

bool event::wait_condition()
{
    return _event_signaled;
}

void *event::on_wait()
{
    return NULL;
}

unsigned long init_tick_count()
{
    timeval use;
    gettimeofday(&use, NULL);
    return use.tv_sec * 1000000 + use.tv_usec;
}

unsigned long current_tick_count()
{
    static unsigned long start = init_tick_count();
    timeval use;
    gettimeofday(&use, NULL);
    return use.tv_sec * 1000000 + use.tv_usec - start;
}

int event::wait(unsigned int ms)
{
    int ret = 0;
    event_enter();
    unsigned long start = current_tick_count();
    if ( ms )
    {
        unsigned long passed = 0;
        unsigned long us = ms * 1000;

        // 如果超时或者事件可用，则不用睡眠在cond_wait上，否则使用cond_wait休眠，cond_wait可以随时被唤醒
        while ( !wait_condition() && (passed < us))
        {
            unsigned long int left = us - passed;
            printf("timeout=%lu spend=%lu left=%lu\n", us, passed, left);
            unsigned long int sec = left / 1000000;
            unsigned long int nsec = left % 1000000 * 1000;
            timespec to;
            clock_gettime(CLOCK_REALTIME, &to);
            printf("now=%lu:%lu", (unsigned long int) to.tv_sec, (unsigned long int) to.tv_nsec);
            to.tv_sec += sec;
            to.tv_nsec += nsec;
            if ( to.tv_nsec >= 1000000000 )
            {
                to.tv_nsec -= 1000000000;
                to.tv_sec++;
            }
            printf("act=wait_event timeout_time=%lu:%lu this=%p\n", (unsigned long int) to.tv_sec,
                   (unsigned long int) to.tv_nsec, this);
            // 如果被唤醒，则ret必然是0，此时如果事件可用，则返回值就是0；如果超时，则返回值是非0
            // 如果超时，但是超时后事件到来，貌似有问题
            ret = pthread_cond_timedwait(&_event_cond, &_event_mutex, &to);
            if ( ret )
            {
                printf("act=wait_error\n");
                passed = current_tick_count() - start;
            }
            else
            {
                printf("act=wait_succ this=%p\n", this);
            }
        }
    }
    else
    {
        while ( !wait_condition())
        {
            pthread_cond_wait(&_event_cond, &_event_mutex);
        }
    }
    if ( _event_auto_reset )
    {
        reset_event();
    }
    return ret;
}

awe_mutex::awe_mutex(int attribute) : _mutex(NULL)
{
    _mutex = new pthread_mutex_t;
    if ( _mutex )
    {
        pthread_mutexattr_t attr;
        if ((0 != pthread_mutexattr_init(&attr)) ||
            (0 != pthread_mutexattr_settype(&attr, attribute)) ||
            (0 != pthread_mutex_init(_mutex, &attr)))
        {
            delete _mutex;
            _mutex = NULL;
        }
    }
}

awe_mutex::~awe_mutex()
{
    if ( _mutex )
    {
        pthread_mutex_destroy(_mutex);
        delete _mutex;
    }
    _mutex = NULL;
}

int awe_mutex::lock()
{
    return pthread_mutex_lock(_mutex);
}

int awe_mutex::unlock()
{
    return pthread_mutex_unlock(_mutex);
}

awe_mutex::operator bool()
{
    return (_mutex != NULL);
}

awe_mutex::operator pthread_mutex_t *()
{
    return _mutex;
}

awe_mutex_helper::awe_mutex_helper(pthread_mutex_t *mutex)
        : _mutex(mutex)
{
    if ( _mutex )
    {
        pthread_mutex_lock(_mutex);
    }
}

awe_mutex_helper::~awe_mutex_helper()
{
    if ( _mutex )
    {
        pthread_mutex_unlock(_mutex);
    }
}

awe_rw_mutex::awe_rw_mutex(bool wf) : _mutex(NULL)
{
    _mutex = new pthread_rwlock_t;
    if ( _mutex )
    {
        int result = 0;
        if ( wf )
        {
            result = pthread_rwlock_init(_mutex, NULL);
        }
        else
        {
            pthread_rwlockattr_t attr = {PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, PTHREAD_PROCESS_PRIVATE};
            result = pthread_rwlock_init(_mutex, &attr);
        }
        if ( result )
        {
            delete _mutex;
            _mutex = NULL;
        }
    }
}

awe_rw_mutex::~awe_rw_mutex()
{
    if ( _mutex )
    {
        pthread_rwlock_destroy(_mutex);
        delete _mutex;
    }
    _mutex = NULL;
}

int awe_rw_mutex::rlock()
{
    return pthread_rwlock_rdlock(_mutex);
}

int awe_rw_mutex::runlock()
{
    return pthread_rwlock_unlock(_mutex);
}

int awe_rw_mutex::wlock()
{
    return pthread_rwlock_wrlock(_mutex);
}

int awe_rw_mutex::wunlock()
{
    return pthread_rwlock_unlock(_mutex);
}

awe_rw_mutex::operator bool()
{
    return (_mutex != NULL);
}

awe_rw_mutex::operator pthread_rwlock_t *()
{
    return _mutex;
}

awe_mutex_locker::awe_mutex_locker(awe_mutex &mutex) : _mutex(mutex)
{
    if ( _mutex )
    {
        _mutex.lock();
    }
}

awe_mutex_locker::~awe_mutex_locker()
{
    if ( _mutex )
    {
        _mutex.unlock();
    }
}

awe_read_locker::awe_read_locker(awe_rw_mutex &mutex) : _mutex(mutex)
{
    if ( _mutex )
    {
        _mutex.rlock();
    }
}

awe_read_locker::~awe_read_locker()
{
    if ( _mutex )
    {
        _mutex.runlock();
    }
}

awe_write_locker::awe_write_locker(awe_rw_mutex &mutex) : _mutex(mutex)
{
    if ( _mutex )
    {
        _mutex.wlock();
    }
}

awe_write_locker::~awe_write_locker()
{
    if ( _mutex )
    {
        _mutex.wunlock();
    }
}
