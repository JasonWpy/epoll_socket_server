#include <zconf.h>
#include <poll.h>
#include "socket.h"

do_socket_event reg_sock_event_proc(do_socket_event proc)
{
    socket_global::get_instance().reg_sock_event_proc(proc);
    return NULL;
}

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
               _epoll, nfds, errno, strerror(errno));
    }
    return true;
}


socket_global::sg_map_t socket_global::_sgs;


socket_global::socket_global(unsigned int threads, do_socket_event proc) :
        _threads(threads)
{
    if ( _threads == 0 )
    {
        _threads = 1;
    }
    for ( unsigned int i = 0; i < _threads; ++i )
    {
        event_complete *use = new event_complete(proc);
        use->start();
        _epolls.push_back(use);
    }
}

socket_global::~socket_global()
{

}

socket_global &socket_global::get_instance(unsigned int threads, do_socket_event proc, const char *name)
{
    socket_global *ret = NULL;
    sg_map_t::const_iterator iter = _sgs.find(name);
    if ( iter != _sgs.end())
    {
        ret = iter->second;
    }
    else
    {
        ret = new socket_global(threads, proc);
        _sgs.insert(std::make_pair(name, AutoPtr<socket_global>(ret)));
    }
    return *ret;
}

socket_global &socket_global::get_instance(const char *name)
{
    return get_instance(8, NULL, name);
}

int socket_global::set_wait_event_count(unsigned int count)
{
    std::vector<AutoPtr < event_complete> > ::iterator
    iter;
    for ( iter = _epolls.begin(); iter != _epolls.end(); iter++ )
    {
        (*iter)->set_wait_event_count(count);
    }
    return 0;
}

int socket_global::reg_sock_event_proc(do_socket_event proc)
{
    std::vector<AutoPtr < event_complete> > ::iterator
    iter;
    for ( iter = _epolls.begin(); iter != _epolls.end(); iter++ )
    {
        (*iter)->reg_sock_event_proc(proc);
    }
    return 0;
}

int socket_global::add_check(int sock, unsigned int events, void *data, bool et)
{
    int ret = -1;
    event_complete *use = _epolls[sock % _threads];
    if ( use )
    {
        ret = use->add_check(sock, events, data, et);
    }
}
int socket_global::update_check(int sock, unsigned int events, void *data, bool et)
{
    int ret = -1;
    event_complete *use = _epolls[sock % _threads];
    if ( use )
    {
        ret = use->update_check(sock, events, data, et);
    }
    return ret;
}
int socket_global::remove(int sock)
{
    int ret = -1;
    event_complete *use = _epolls[sock % _threads];
    if ( use )
    {
        ret = use->remove(sock);
    }
    return ret;
}


sock_event::sock_event() :
        _head_over(false),
        _wish(sizeof(header_t)),
        _count(0),
        _sock(-1),
        _buff(NULL)
{
    memset(&_remote, 0, sizeof(sockaddr_in));
}

sock_event::~sock_event()
{
    if ( _buff )
    {
        delete[]_buff;
        _buff = NULL;
    }
}
int sock_event::set_socket(int sock, const sockaddr_in &addr)
{
    _sock = sock;
    _remote = addr;
    return 0;
}
int sock_event::on_shut_done()
{
    static socket_global *sg = socket_global::get_instance();
    printf("sock_event:shutdown fd=%d peer=0x%x:%u",
           _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
    sg->remove(_sock);
    close(_sock);
    _sock = -1;
    return -1;//TODO do not make scene
}
int sock_event::on_data_in()
{
    int ret = 0;
    int result = 0;
    do
    {
        result = read_command();
        printf("sock_event:read readCommand_return ret=%d fd=%d peer=0x%x:%u",
               result, _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
        switch ( result )
        {
            case 0:
                // TODO: 对方关闭连接
                on_shut_done();
                ret = -1;
                break;
            case 1:
            {
                // TODO: 调用Command处理函数
                // 需要注意的是，onCommand中请不要对传入的m_buff进行释放
                // 如果onCommand需要保存该数据，请自己创建缓冲区另外保存
                int ret_on_cmd = 0;
                ret_on_cmd = on_command(_head, _buff, _head.length);
                ret = 0;
                break;
            }
            case 2:
                //TODO: 没有读完一个完整的命令
                break;
            default:
                //TODO: 网络出错
                on_shut_done();
                ret = -1;
                break;
        }
    } while ((1 == result) && more_data());
    return ret;
}
int sock_event::on_data_out()
{
    int ret = 0;
    if ( _count )
    {
        awe_mutex_locker locke(_mutex);
        if ( _count )
        {
            AutoPtr<unsigned char> item = _send_q.front();
            _send_q.pop();
            if ( item )
            {
                send(_sock, (unsigned char *) item, item.size(), 0);
            }
            _count--;
        }
    }
    return ret;
}
int sock_event::on_error()
{
    on_shut_done();
    return -1;
}
int sock_event::on_hung_up()
{
    on_shut_done();
    return -1;
}
int sock_event::on_urgent_data()
{
    int ret = 0;
    return ret;
}
int sock_event::on_sock_event(int events)
{
    printf("sock_event:entry fd=%d events=0x%x peer=%x:%u",
           _sock, events, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
    int ret = 0;
    if ( !ret && (events & EPOLLERR))
    {
        ret = on_error();
    }
    if ( !ret && (events & EPOLLHUP))
    {
        ret = on_hung_up();
    }
    if ( !ret && (events & EPOLLPRI))
    {
        ret = on_urgent_data();
    }
    if ( !ret && ((events & EPOLLRDNORM) || (events & EPOLLIN)))
    {
        ret = on_data_in();
    }
    if ( !ret && ((events & EPOLLWRNORM) || (events & EPOLLOUT)))
    {
        ret = on_data_out();
    }
    return ret;
}
int sock_event::sync_send(const unsigned char *buff, unsigned int length)
{
    int ret = 0;
    if ((length > 0) && buff )
    {
        AutoPtr<unsigned char> item(length);
        memcpy((unsigned char *) item, buff, length);
        awe_mutex_locker locke(_mutex);
        _send_q.push(item);
        _count++;
    }
    return ret;
}


int sock_event::check_header(const header_t &header)
{
    int ret = 0;
    if ( header.code != SOME_CODE ) //TODO SOME_CODE is define somewhere
    {
        ret = -1;
    }
    return ret;
}

int sock_event::on_command(const header_t &header, unsigned char *buff, unsigned int length)
{
    return 0;
}

// 返回值
// -1 : 错误
//  0 : 没有读取到数据
//  1 : 完整
//  2 : 未完整
int sock_event::read_command()
{
    const size_t ERR_MSG_BUF_LENGTH = 256;
    char errSysMsg[ERR_MSG_BUF_LENGTH] = "";
    int32_t ret = 0;
    if ( _head_over )
    {
        //TODO: 读取body
        if ( _wish == _head.length )
        {
            if ( _buff )
            {
                delete[]_buff;
                _buff = NULL;
            }
            _buff = new unsigned char[_head.length];
            if ( _buff )
            {
                printf("error sockEvent_malloc_fail length=%d fd=%d peer=%x:%u",
                       _head.length, _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
                return -1;
            }
        }

        ret = recv(_sock, _buff + (_head.length - _wish), _wish, 0);
        if ( ret == 0 )
        {
            printf("sock_event:read peer_close_socket fd=%d peer=%x:%u ret=%d",
                   _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port), ret);
            return 0;
        }

        if ((unsigned int) ret != _wish )
        {
            _wish = _wish - (unsigned int) ret;
            return 2;
        }

        //读取了一个完整的包.
        _header_over = false;
        _wish = sizeof(header_t);
        return 1;
    }
    else
    {
        // TODO: 读取Header
        uint8_t *use = (uint8_t *) &_head;
        ret = recv(_sock, use + (sizeof(header_t) - _wish), _wish, 0);
        if ( 0 == ret )
        {
            printf("sock_event:read when=read_header err=peer_close_socket"
                   " fd=%d peer=0x%x:%u ret=%d", _sock, _remote.sin_addr.s_addr,
                   ntohs(_remote.sin_port), ret);
            return 0;
        }
        else if ( ret < 0 )
        {
            printf("sock_evet:read_header_fail ret=%d errno=%d"
                   " fd=%d peer=0x%x:%u",
                   ret, errno, _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
            return -1;
        }

        if ((uint32_t) ret != _wish )
        {
            _wish = _wish - (uint32_t) ret;
            return 2;
        }

        printf("sock_event:read got_full_header"
               " req_id=%x:%u:%lu opcode=%x syncode=0x%x  length=%u"
               " param64=%lu param32=%u fd=%d peer=%x:%u",
               _head.ip, ntohs(_head.port), _head.id, _head.opcode, _head.syncode,
               _head.length, _head.req_param64, _head.req_param32,
               _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));

        if ( check_header(_head))
        {
            return -1;
        }

        // TODO: 读取Body
        _header_over = true;
        _wish = _head.length;
        if ( _wish <= 0 )
        {
            _header_over = false;
            _wish = sizeof(header_t);
            printf("act=dump_packet head=\n%s packet=\n%s",
                   DumpMemory(&_head, sizeof(_head)).c_str(),
                   DumpMemory(_buff, _head.length).c_str());
            printf("sock_event:read got_full_packet fd=%d peer=0x%x:%u "
                   "req_id=0x%x:%u:%lu opcode=0x%x length=%u",
                   _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port),
                   _head.ip, ntohs(_head.port), _head.id, _head.opcode, _head.length);
            return 1;
        }
        if ( !more_data())
        {
            return 2;
        }

        if ( NULL != _buff )
        {
            delete[]_buff;
            _buff = NULL;
        }
        _buff = new unsigned char[_wish];
        if ( _buff == NULL )
        {
            printf("sockEvent_malloc_fail length=%d fd=%d peer=%x:%u",
                   _head.length, _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port));
            return -1;
        }

        ret = recv(_sock, _buff + (_head.length - _wish), _wish, 0);
        if ( ret == 0 )
        {
            //连接被关闭.
            printf("sock_event:read peer_close_socket fd=%d peer=0x%x:%u ret=%d",
                   _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port), ret);
            return 0;
        }
        else if ( ret < 0 )
        {
            printf("sock_event:read recv_body_fail"
                   " fd=%d peer=%x:%u ret=%d errno=%d sys_desc=%s",
                   _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port),
                   ret, errno, strerror_r(errno, errSysMsg, ERR_MSG_BUF_LENGTH));
            return -1;
        }

        _head_over = false;
        _wish = sizeof(header_t);
        printf("act=dump_packet head=\n%s packet=\n%s",
               DumpMemory(&_head, sizeof(_head)).c_str(),
               DumpMemory(_buff, _head.length).c_str());
        printf("sock_event:read got_full_packet fd=%d peer=0x%x:%u"
               " req_id=0x%x:%u:%lu opcode=0x%x length=%u",
               _sock, _remote.sin_addr.s_addr, ntohs(_remote.sin_port),
               _head.ip, ntohs(_head.port), _head.id, _head.opcode, _head.length);
        return 1;

    }


    return 0;
}
bool sock_event::more_data(unsigned int ms)
{
    struct pollfd mypfd;
    mypfd.fd = _sock;
    mypfd.events = POLLIN | POLLRDNORM;

    for( ; ; )
    {
        int result = poll(&mypfd, 1, ms);
        if(( -1 == result ) && ( EINTR == errno ))
        {///< handle the system interupts >
            continue;
        }

        if(result > 0 && ((mypfd.revents & POLLIN) || (mypfd.revents & POLLRDNORM)))
        {
            return true;
        }

        return false;
    }
}



