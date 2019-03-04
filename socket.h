#ifndef EPOLL_SOCKET_SERVER_SOCKET_H
#define EPOLL_SOCKET_SERVER_SOCKET_H

#include <string>
#include <map>
#include <sys/epoll.h>
#include <netinet/in.h>
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
    typedef std::map<std::string, AutoPtr < socket_global> >sg_map_t;

private:
    awe_mutex _locker_use_mutex;
public:
    operator awe_mutex &()
    {
        return _locker_use_mutex;
    }

private:
    static sg_map_t _sgs;
    unsigned int _threads;
    awe_mutex _mutex;
    std::vector<AutoPtr<event_complete> > _epolls;

public:
    ~socket_global();
    static socket_global &get_instance(unsigned int threads = 8, do_socket_event proc = NULL, const char *name = "");
    static socket_global &get_instance(const char *name);

    int set_wait_event_count(unsigned int count);
    int add_check(int sock, unsigned int events, void *data, bool et = true);
    int update_check(int sock, unsigned int events, void *data, bool et = true);
    int remove(int sock);

private:
    socket_global(unsigned int threads, do_socket_event proc);
    socket_global(const socket_global &);
    socket_global &operator=(const socket_global &);

    int reg_sock_event_proc(do_socket_event proc);

public:
    friend do_socket_event reg_sock_event_proc(do_socket_event proc);
};


class sock_event
{
public:
    sock_event();
    virtual ~sock_event();
    virtual int set_socket(int sock, const sockaddr_in &addr);
    virtual int on_shut_done();
    virtual int on_data_in();
    virtual int on_data_out();
    virtual int on_error();
    virtual int on_hung_up();
    virtual int on_urgent_data();
    virtual int on_sock_event(int events);
    virtual int sync_send(const unsigned char *buff, unsigned int length);

protected:
    virtual int check_header(const header_t &header);
    virtual int on_command(const header_t &header, unsigned char *buff, unsigned int length);
    virtual int read_command();
    bool more_data(unsigned int ms = 0);
protected:
    bool _head_over;
    unsigned int _wish;
    unsigned int _count;
    std::queue<AutoPtr<unsigned char> > _send_q;
    awe_mutex _mutex;
    sockaddr_in _remote;

protected:
    int _sock;
    header_t _head;
    unsigned char *_buff;

};

#endif //EPOLL_SOCKET_SERVER_SOCKET_H
