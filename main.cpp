#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

int m_pipefd_[2];
int epollfd_;
#define MAX_EVENT_NUMBER 100
epoll_event events_[MAX_EVENT_NUMBER];

void sig_handler(int sig) {
	std::cout << "sig_handler: " << sig << std::endl;
	send(m_pipefd_[1], (char*)&sig, 1, 0);
}

void addsig(int sig) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = sig_handler;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

int fd_setnonblock(int fd) {
	int old_fl = fcntl(fd, F_GETFL);
	int new_fl = old_fl | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_fl);
	return old_fl;
}

void addfd(int epollfd, int fd) {
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET | EPOLLHUP;
	event.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	fd_setnonblock(fd);
}

void eventLoop() {
	bool stop_server = false;
	while (!stop_server) {
		int number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1);
		if (number < 0 && errno != EINTR) {
			break;
		}
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

void setup_epoll() {
	epollfd_ = epoll_create(5);
	assert(epollfd_ != -1);

	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd_);
	assert(ret != -1);

	addfd(epollfd_, m_pipefd_[0]);
	fd_setnonblock(m_pipefd_[1]);
}

int main() {
	std::cout << "Server start..." << std::endl;
	addsig(SIGALRM);
	setup_epoll();
	alarm(5);
	eventLoop();
}
