#ifndef EPOLL_SOCKET_SERVER_AWE_MUTEX_H
#define EPOLL_SOCKET_SERVER_AWE_MUTEX_H

#include <pthread.h>

class event
{
public:
    virtual ~event();

    static event *create_event(bool auto_reset = false, bool init = false);

    virtual int set_event();

    virtual int reset_event();

    int wait(unsigned int ms = 0);

protected:
    event(bool auto_reset = false, bool init = false);

    void event_enter();

    void event_leave();

    virtual bool wait_condition();

    virtual void *on_wait();

private:
    pthread_mutex_t _event_mutex;
    pthread_cond_t _event_cond;
    bool _event_signaled;
    bool _event_auto_reset;
};

class awe_mutex
{
public:
    awe_mutex(int attribute = PTHREAD_MUTEX_NORMAL);

    virtual ~awe_mutex();

    virtual int lock();

    virtual int unlock();

    operator bool();

    operator pthread_mutex_t *();

private:
    awe_mutex(const awe_mutex &);

    awe_mutex &operator=(const awe_mutex &);

protected:
    pthread_mutex_t *_mutex;

};

class awe_mutex_helper
{
public:
    awe_mutex_helper(pthread_mutex_t *mutex);

    ~awe_mutex_helper(void);

private:
    awe_mutex_helper(const awe_mutex_helper &);

    awe_mutex_helper &operator=(const awe_mutex_helper &);

    pthread_mutex_t *_mutex;

};

// RWMutex : 默认为写优先
class awe_rw_mutex
{

public:
    explicit awe_rw_mutex(bool wf = true);

    ~awe_rw_mutex();

    int rlock();

    int runlock();

    int wlock();

    int wunlock();

    operator bool();

    operator pthread_rwlock_t *();

private:
    awe_rw_mutex(const awe_rw_mutex &);

    awe_rw_mutex &operator=(const awe_rw_mutex &);

    pthread_rwlock_t *_mutex;
};

class awe_mutex_locker
{

public:
    explicit awe_mutex_locker(awe_mutex &mutex);

    ~awe_mutex_locker();

private:
    awe_mutex &_mutex;
};

class awe_read_locker
{
public:
    explicit awe_read_locker(awe_rw_mutex &mutex);

    ~awe_read_locker();

private:
    awe_rw_mutex &_mutex;

};

class awe_write_locker
{

public:
    explicit awe_write_locker(awe_rw_mutex &mutex);

    ~awe_write_locker();

private:
    awe_rw_mutex &_mutex;
};


#endif //EPOLL_SOCKET_SERVER_AWE_MUTEX_H
