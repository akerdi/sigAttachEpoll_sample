# Signal Attach Epoll

Sample for showing case about Linux Signal (SIGALRM) callback then epoll handle all signal together.

## ENVIRONMENT

```
platform: Linux
language: C++11
ide     : Clion
```

## Process

`int(*sigfillset)(sigset_t* set)`:

initializes set to full, including all signals.

`int(*sigaction)(int sig,struct sigaction* act, struct sigaction* oldact)`:

examine and change a signal action. set `sig` to new `act`, get `oldact` if exists.

```cpp
void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
```
`addsig` helps sig setup sig to action as `sa`, it response to new handler.

`int(*socketpair)(int domain, int type, int protocol, int sv[2])`:

create a pair of connected sockets. `sv[0]` for read, and `sv[1]` is for write.

```cpp
void setup_epoll() {
    epollfd_ = epoll_create(0);
    assert(epollfd_ != -1);
    
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd_);
    assert(ret != -1);
    
    addfd(epollfd_, m_pipefd_[0]);
    fd_setnonblock(m_pipefd_[1]);
}
```

`setup_epoll` first create epoll, then create socketpair that make m_pipefd_ array to connect each other. After that, add m_pipefd_[0] to epoll, finally set m_pipefd_[1] to nonblock.

```cpp
void eventLoop() {
    bool stop_server = false;
    while (!stop_server) {
        int number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER);
        if (number < 0 && errno != EINTR)
            break;
        for (int i = 0; i < number; i++) {
            int sockfd = events_[i].data.fd;
            if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                std::cout << "Error" << std::endl;
            } else if (sockfd == m_pipefd_[0]) {
                puts("Receive signal!");
                alarm(5);
            }
        }   
    }
}
```
`void(*eventLoop)(void)` is a block loop, which retrieve notification from kernel in indefinite mode. When `sockfd` is equal to m_pipefd_[0], it is sure the signal came from the `SIGALRM`!

## Ref

[epoll_socket](https://github.com/akerdi/epoll_socket)