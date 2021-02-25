#pragma once

#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace lon::io {
	void setHookEnabled(bool enable);
    bool isHookEnabled();
	void hook_init();
}

extern "C" {

	//sleep
	typedef unsigned int (*sleep_fun)(unsigned int seconds);
	extern sleep_fun sleep_sys;
	
	typedef int (*usleep_fun)(useconds_t usec);
	extern usleep_fun usleep_sys;
	
	typedef int (*nanosleep_fun)(const struct timespec* req, struct timespec* rem);
	extern nanosleep_fun nanosleep_sys;
	
	//socket
	typedef int (*socket_fun)(int domain, int type, int protocol);
	extern socket_fun socket_sys;
	
	typedef int (*connect_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
	extern connect_fun connect_sys;
	
	typedef int (*accept_fun)(int s, struct sockaddr* addr, socklen_t* addrlen);
	extern accept_fun accept_sys;
	
	//read
	typedef ssize_t(*read_fun)(int fd, void* buf, size_t count);
	extern read_fun read_sys;
	
	typedef ssize_t(*readv_fun)(int fd, const struct iovec* iov, int iovcnt);
	extern readv_fun readv_sys;
	
	typedef ssize_t(*recv_fun)(int sockfd, void* buf, size_t len, int flags);
	extern recv_fun recv_sys;
	
	typedef ssize_t(*recvfrom_fun)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
	extern recvfrom_fun recvfrom_sys;
	
	typedef ssize_t(*recvmsg_fun)(int sockfd, struct msghdr* msg, int flags);
	extern recvmsg_fun recvmsg_sys;
	
	//write
	typedef ssize_t(*write_fun)(int fd, const void* buf, size_t count);
	extern write_fun write_sys;
	
	typedef ssize_t(*writev_fun)(int fd, const struct iovec* iov, int iovcnt);
	extern writev_fun writev_sys;
	
	typedef ssize_t(*send_fun)(int s, const void* msg, size_t len, int flags);
	extern send_fun send_sys;
	
	typedef ssize_t(*sendto_fun)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen);
	extern sendto_fun sendto_sys;
	
	typedef ssize_t(*sendmsg_fun)(int s, const struct msghdr* msg, int flags);
	extern sendmsg_fun sendmsg_sys;
	
	typedef int (*close_fun)(int fd);
	extern close_fun close_sys;
	
	//
	typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
	extern fcntl_fun fcntl_sys;
	
	typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
	extern ioctl_fun ioctl_sys;
	
	typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
	extern getsockopt_fun getsockopt_sys;
	
	typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
	extern setsockopt_fun setsockopt_sys;

}