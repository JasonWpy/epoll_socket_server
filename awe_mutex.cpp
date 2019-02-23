#include <sys/time.h>
#include "awe_mutex.h"

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
int event::reset_revent()
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
        }
    }
}

