#include "io/io_manager.h"
#include <unistd.h>
#include <fcntl.h>

namespace lon::io {
constexpr int epoll_create_size = 1000;
Logger::ptr G_Logger = LogManager::getInstance()->getLogger("system");

IoManager::IoManager(size_t thread_count) : scheduler_{thread_count} {
	scheduler_.setExitWithTasksProcessed(true);

	initEpoll();
	initPipe();
}

bool IoManager::registerEvent(int fd,
    EventType type,
    EventCallbackType callback) noexcept {
	if(static_cast<size_t>(fd) > fd_callbacks_.size()) {
        fd_callbacks_.resize(static_cast<size_t>(fd));
	}
    fd_callbacks_[fd] = std::move(callback);
}

void IoManager::initEpoll() {
	epoll_fd_ = epoll_create(epoll_create_size);
	if (epoll_fd_ == -1) {
		LON_LOG_ERROR(G_Logger) << fmt::format("epoll create failed, with error: {}", std::strerror(errno));
		assert(false);
	}
}

void IoManager::initPipe() {
	int ret = pipe(wakeup_pipe_fd_);
	// if (ret == -1) {
	// 	LON_LOG_ERROR(G_Logger) << fmt::format("pipe open failed, with error: {}", std::strerror(errno));
	// 	assert(false);
	// }
	LON_ERROR_INVOKE_ASSERT(ret != -1, "pipe open", G_Logger);

	struct epoll_event ep_event;
	bzero(&ep_event, sizeof(struct epoll_event));
	ep_event.events = EPOLLIN | EPOLLOUT;
	ep_event.data.fd = wakeup_pipe_fd_[0];
	ret = fcntl(wakeup_pipe_fd_[0], F_SETFL, O_NONBLOCK);
	// if (ret == -1) {
	// 	LON_LOG_ERROR(G_Logger) << fmt::format("fcntl failed, with error: {}", std::strerror(errno));
	// 	assert(false);
	// }
	LON_ERROR_INVOKE_ASSERT(ret != -1, "fcntl", G_Logger);

	ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_pipe_fd_[0], &ep_event);
	// if (ret == -1) {
	// 	LON_LOG_ERROR(G_Logger) << fmt::format("epoll_ctl open failed, with error: {}", std::strerror(errno));
	// 	assert(false);
	// }
	LON_ERROR_INVOKE_ASSERT(ret != -1, "epoll_ctl", G_Logger);
}
}
