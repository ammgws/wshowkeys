#ifndef _DEVMGR_H
#define _DEVMGR_H

int devmgr_start(int *fd, pid_t *pid, const char *devpath);
int devmgr_open(int sockfd, const char *path);
void devmgr_finish(int sock, pid_t pid);

#endif
