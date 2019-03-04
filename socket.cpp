#include "socket.h"

event_complete::event_complete(do_socket_event proc) :
        _count(50),
        _proc(proc)
{
    _epoll = epoll_create(200000);
}

event_complete::~event_complete()
{
}

void event_complete::set_wait_event_count(unsigned int count)
{
    _count = count;
}

void event_complete::set_sock_event_proc(do_socket_event proc)
{
    _proc = proc;
}

int event_complete::add_check(int sock, unsigned int events, void *data, bool et)
{
    struct epoll_event ev;
    ev.data.ptr = data;
    ev.events = events;
    if ( ev )
    {
        ev.events |= EPOLLET;
    }
    int ret = epoll_ctl(_epoll, EPOLL_CTL_ADD, sock, &ev);
    printf("epoll:add fd=%d epoll=%d ret=%d et=%d ptr=%p\n",
           sock, _epoll, ret, et ? 1 : 0, data);
    if ( ret )
    {
        printf("epoll:add fd=%d epoll=%d ret=%d errno=%d desc=%s",
               sock, _epoll, ret, errno, strerror(errno));
    }
    return ret;
}

int event_complete::update_check(int sock, unsigned int events, void *data, bool et)
{
    struct epoll_event ev;
    ev.data.ptr = data;
    ev.events = events;
    if ( et )
    {
        ev.events |= EPOLLET;
    }
    int ret = epoll_ctl(_epoll, EPOLL_CTL_MOD, sock, &ev);
    printf("epoll:add fd=%d epoll=%d ret=%d et=%d ptr=%p",
           sock, _epoll, ret, et ? 1 : 0, data);
}

int event_complete::remove(int sock)
{
    int ret = epoll_ctl(_epoll, EPOLL_CTL_DEL, sock, NULL);
    printf("epoll:remove fd=%d epoll=%d ret=%d ", sock, _epoll, ret);
    return ret;
}

#define MAX_WAITED_EVENT 10

bool event_complete::run()
{
    epoll_event events[MAX_WAITED_EVENT];
    int nfds;
    nfds = epoll_wait(_epoll, events, MAX_WAITED_EVENT, -1);
    if ( 0 == nfds )
    {
        printf("epoll:run epoll=%d event_num=0\n", _epoll);
        usleep(10);
    }
    else if ( nfds > 0 )
    {
        printf("epoll:run epoll=%d event_num=%d\n", _epoll, nfds);
        if ( _proc )
        {
            for ( int i = 0; i < nfds; ++i )
            {
                _proc(events[i].data.ptr, events[i].events);
            }
        }
    }
    else
    {
        //这里可能被信号打断，依赖线程再次循环导致epoll_wait
        printf("epoll:run epoll=%d event_num=%d errno=%d sys_desc=%s\n",
               m_epoll, nfds, errno, strerror(errno));
    }
    return true;
}


