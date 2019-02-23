#ifndef EPOLL_SOCKET_SERVER_THREAD_H
#define EPOLL_SOCKET_SERVER_THREAD_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <new>

#define AVOID_UNUSED_VAR_WARN(param) ((void)(param))

template<typename T>
class awe_thread
{
protected:
    bool _thread_run;
    pthread_t _thread_tid;
    bool _thread_joinable;

public:
    awe_thread(bool joinable = true) :
            _thread_run(false),
            _thread_joinable(joinable)
    {
    }

    virtual ~awe_thread()
    {
        stop();
    }

    virtual bool start()
    {
        bool ret = true;
        if ( !_thread_run )
        {
            _thread_run = true;
            pthread_attr_t attr;
            pthread_attr_t *attr_arg = NULL;

            if ( !_thread_joinable )
            {
                attr_arg = &attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            }

            int result = pthread_create(&_thread_tid, attr_arg, thread_routine, this);
            if ( result )
            {
                ret = false;
            }
            if ( ret )
            {
                printf("thread start success\n");
            }
            else
            {
                printf("thread start fail err=%s\n", strerror(result));
            }
        }
        else
        {
            printf("thread already running\n");
        }
        return ret;
    }

    virtual bool stop(int timeout = 0)
    {
        AVOID_UNUSED_VAR_WARN(timeout);
        bool ret = true;
        if ( _thread_run )
        {
            _thread_run = false;

        }
        return ret;
    }
    bool pause()
    {
        return !_thread_run;
    }
    bool resume(void)
    {
        return _thread_run;
    }
    bool wait(int timeout = 0)
    {
        int result = pthread_join(_thread_tid, NULL);
        return (0 == result);
    }
    unsigned int get_thread_id()
    {
        return _thread_tid;
    }

    virtual int on_exception()
    {
        return 0;
    }
    virtual int on_thread_start()
    {
        return 0;
    }
    virtual int on_thread_stop()
    {
        return 0;
    }
    virtual bool run()
    {
        return _thread_run;
    }

protected:
    static void *thread_routine(void *arg)
    {
        T *obj = static_cast<T *>(arg);
        obj->on_thread_start();
        try
        {
            while ( obj->_thread_run && obj->run())
            {
            }
        } catch (const std::bad_alloc &)
        {
            obj->on_exception();
        } catch (...)
        {
            obj->on_exception();
        }
        obj->_thread_run = false;
        obj->on_thread_stop();
        return 0;
    }
};


template<typename T>
class awe_dual_thread
{
protected:
    bool _thread_run;
    pthread_t _thread1_tid;
    pthread_t _thread2_tid;
    bool _thread_joinable;

public:
    awe_dual_thread(bool joinable = true) :
            _thread_run(false),
            _thread_joinable(joinable)
    {
    }
    virtual ~awe_dual_thread()
    {
        stop();
    }
    bool start()
    {
        bool ret = true;
        if ( !_thread_run )
        {
            _thread_run = true;
            pthread_attr_t attr;
            pthread_attr_t *attr_arg = NULL;
            if ( !_thread_joinable )
            {
                attr_arg = &attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            }
            int result = pthread_create(&_thread1_tid, attr_arg, thread1_routine, this);
            if ( result )
            {
                ret = false;
            }
            if ( ret )
            {
                result = pthread_create(&_thread2_tid, attr_arg, thread2_toutine, this);
                if ( result )
                {
                    ret = false;
                }
            }
            if ( ret )
            {
                printf("thread start success\n");
            }
            else
            {
                printf("thread start fail err=%s\n", strerror(result));
            }
        }
        else
        {
            printf("thread already running\n");
        }
        return ret;
    }
    bool stop(int timeout = 0)
    {
        AVOID_UNUSED_VAR_WARN(timeout);
        bool ret = true;
        if ( _thread_run )
        {
            _thread_run = false;
        }
        return ret;
    }
    bool pause()
    {
        return !_thread_run;
    }
    bool resume()
    {
        return _thread_run;
    }
    bool wait(int timeout = 0)
    {
        int result = pthread_join(_thread2_tid, NULL);
        return (0 == result);
    }
    unsigned int get_thread1_id(void)
    {
        return _thread1_tid;
    }
    unsigned int get_thread2_id(void)
    {
        return _thread2_tid;
    }
    virtual int32_t on_thread1_exception(void)
    {
        return 0;
    };
    virtual int32_t on_thread2_exception(void)
    {
        return 0;
    };
    virtual int32_t on_thread1_start(void)
    {
        return 0;
    };
    virtual int32_t on_thread2_start(void)
    {
        return 0;
    };
    virtual int32_t on_thread1_stop(void)
    {
        return 0;
    };
    virtual int32_t on_thread2_stop(void)
    {
        return 0;
    };
    virtual bool run1(void)
    {
        return true;
    };
    virtual bool run2(void)
    {
        return true;
    };

protected:
    static void *thread1_routine(void *arg)
    {
        T *obj = static_cast<T *>(arg);
        obj->on_thread_start();
        try
        {
            while ( obj->_thread_run && obj->run())
            {
            }
        } catch (const std::bad_alloc &)
        {
            obj->on_exception();
        } catch (...)
        {
            obj->on_excepthon();
        }
        obj->_thread_run = false;
        obj->on_thread_stop();
        return 0;
    }
    static void *thread2_routione(void *arg)
    {
        T *obj = static_cast<T *>(arg);
        obj->on_thread_start();
        try
        {
            while ( obj->_thread_run && obj->run())
            {
            }
        } catch (std::bad_alloc &)
        {
            obj->on_exception();
        } catch (...)
        {
            obj->on_exception();
        }
        obj->_thread_run = false;
        obj->on_thread_stop();
        return 0;
    }
};


#endif //EPOLL_SOCKET_SERVER_THREAD_H
