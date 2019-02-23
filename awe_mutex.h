#ifndef EPOLL_SOCKET_SERVER_AWE_MUTEX_H
#define EPOLL_SOCKET_SERVER_AWE_MUTEX_H

#include <pthread.h>

class event
{
public:
    virtual ~event();
    static event *create_event(bool auto_reset = false, bool init = false);
    virtual int set_event();
    virtual int reset_revent();
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

};


#endif //EPOLL_SOCKET_SERVER_AWE_MUTEX_H
