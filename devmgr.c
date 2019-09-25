/*
 * Portions of this file taken from wlroots; MIT licensed. Its purpose is to
 * run a child process as root for opening evdev devices.
 *
 * NOTICE: Most of this code runs as root.
 */
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "devmgr.h"

enum msg_type {
	MSG_OPEN,
	MSG_END,
};

struct msg {
	enum msg_type msg_type;
	char path[PATH_MAX];
};

static ssize_t recv_msg(int sock, int *fd_out, void *buf, size_t buf_len) {
	char control[CMSG_SPACE(sizeof(*fd_out))] = {0};
	struct iovec iovec = { .iov_base = buf, .iov_len = buf_len };
	struct msghdr msghdr = {0};

	if (buf) {
		msghdr.msg_iov = &iovec;
		msghdr.msg_iovlen = 1;
	}

	if (fd_out) {
		msghdr.msg_control = &control;
		msghdr.msg_controllen = sizeof(control);
	}

	ssize_t ret;
	do {
		ret = recvmsg(sock, &msghdr, MSG_CMSG_CLOEXEC);
	} while (ret < 0 && errno == EINTR);

	if (fd_out) {
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr);
		if (cmsg) {
			memcpy(fd_out, CMSG_DATA(cmsg), sizeof(*fd_out));
		} else {
			*fd_out = -1;
		}
	}

	return ret;
}

static void send_msg(int sock, int fd, void *buf, size_t buf_len) {
	char control[CMSG_SPACE(sizeof(fd))] = {0};
	struct iovec iovec = { .iov_base = buf, .iov_len = buf_len };
	struct msghdr msghdr = {0};

	if (buf) {
		msghdr.msg_iov = &iovec;
		msghdr.msg_iovlen = 1;
	}

	if (fd >= 0) {
		msghdr.msg_control = &control;
		msghdr.msg_controllen = sizeof(control);

		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr);
		*cmsg = (struct cmsghdr) {
			.cmsg_level = SOL_SOCKET,
			.cmsg_type = SCM_RIGHTS,
			.cmsg_len = CMSG_LEN(sizeof(fd)),
		};
		memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
	}

	ssize_t ret;
	do {
		ret = sendmsg(sock, &msghdr, 0);
	} while (ret < 0 && errno == EINTR);
}

static void devmgr_run(int sockfd, const char *devpath) {
	struct msg msg;
	int fdin = -1;
	bool running = true;

	while (running && recv_msg(sockfd, &fdin, &msg, sizeof(msg)) > 0) {
		switch (msg.msg_type) {
		case MSG_OPEN:
			errno = 0;
			if (strstr(msg.path, devpath) != msg.path) {
				/* Hackerman detected */
				exit(1);
			}
			int fd = open(msg.path, O_RDONLY|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
			int ret = errno;
			send_msg(sockfd, ret ? -1 : fd, &ret, sizeof(ret));
			if (fd >= 0) {
				close(fd);
			}
			break;
		case MSG_END:
			running = false;
			send_msg(sockfd, -1, NULL, 0);
			break;
		}
	}

	exit(0);
}

int devmgr_start(int *fd, pid_t *pid, const char *devpath) {
	if (geteuid() != 0) {
		fprintf(stderr, "wshowkeys needs to be setuid to read input events\n");
		return 1;
	}

	int sock[2];
	if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sock) < 0) {
		fprintf(stderr, "devmgr: socketpair: %s", strerror(errno));
		return -1;
	}

	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "devmgr: fork: %s", strerror(errno));
		close(sock[0]);
		close(sock[1]);
		return 1;
	} else if (child == 0) {
		close(sock[0]);
		devmgr_run(sock[1], devpath); /* Does not return */
	}
	close(sock[1]);
	*fd = sock[0];
	*pid = child;

	if (setgid(getgid()) != 0) {
		fprintf(stderr, "devmgr: setgid: %s\n", strerror(errno));
		return 1;
	}
	if (setuid(getuid()) != 0) {
		fprintf(stderr, "devmgr: setuid: %s\n", strerror(errno));
		return 1;
	}
	if (setuid(0) != -1) {
		fprintf(stderr, "devmgr: failed to drop root\n");
		return 1;
	}

	return 0;
}

int devmgr_open(int sockfd, const char *path) {
	struct msg msg = { .msg_type = MSG_OPEN };
	snprintf(msg.path, sizeof(msg.path), "%s", path);

	send_msg(sockfd, -1, &msg, sizeof(msg));

	int fd, err, ret;
	int retry = 0;
	do {
		ret = recv_msg(sockfd, &fd, &err, sizeof(err));
	} while (ret == 0 && retry++ < 3);

	return err ? -err : fd;
}

void devmgr_finish(int sock, pid_t pid) {
	struct msg msg = { .msg_type = MSG_END };

	send_msg(sock, -1, &msg, sizeof(msg));
	recv_msg(sock, NULL, NULL, 0);

	waitpid(pid, NULL, 0);

	close(sock);
}
