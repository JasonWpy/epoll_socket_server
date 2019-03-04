#ifndef EPOLL_SOCKET_SERVER_SOCKET_H
#define EPOLL_SOCKET_SERVER_SOCKET_H

#include <string>
#include <map>
#include <sys/epoll.h>
#include "awe_thread.h"
#include "awe_mutex.h"

typedef enum _sock_event
{
    SOCK_EVENT_READ = EPOLLRDNORM,
    SOCK_EVENT_WRITE = EPOLLWRNORM,
    SOCK_EVENT_ERROR = EPOLLERR,
    SOCK_EVENT_HANGUP = EPOLLHUP
} SOCKET_EVENT;

typedef int (*do_socket_event)(void *data, int events);

class event_complete : public awe_thread<event_complete>
{
private:
    event_complete(const event_complete &);
    event_complete &operator=(const event_complete &);

private:
    int _epoll;
    unsigned int _count;
    do_socket_event _proc;

public:
    ~event_complete();
    int add_check(int sock, unsigned int events, void *data, bool et = true);
    int update_check(int sock, unsigned int events, void *data, bool et = true);
    int remove(int sock);

    bool run();

private:
    explicit event_complete(do_socket_event proc = NULL);
    void set_wait_event_count(unsigned int count);
    void set_sock_event_proc(do_socket_event proc);

    friend class socket_global;
};

class socket_global
{
private:
    typedef std::map<std::string, AutoPtr < socket_global> >
    sg_map_t;
private:
    awe_mutex _locker_use_mutex;
public:
    operator awe_mutex &()
    {
        return _locker_use_mutex;
    }

};


#endif //EPOLL_SOCKET_SERVER_SOCKET_H
