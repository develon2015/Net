#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <assert.h>

#define PORT 6621

int
main(int argc, char *argv[]) {
	if (strcmp(argv[1], "safe") == 0) {
		assert(close(STDOUT_FILENO) == 0);
	}
	printf("[I] starting server...\n");
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	struct sockaddr_in lbaddr = { 0 };
	lbaddr.sin_family = AF_INET;
	lbaddr.sin_port = htons(PORT);
	lbaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	if (bind(sockfd, (struct sockaddr *)&lbaddr, sizeof lbaddr) != 0) {
		printf("[E] 绑定端口失败: %s\n", strerror(errno));
		return 1;
	}
	// 被动打开模式
	assert(listen(sockfd, 10) == 0);
	int cfd = 0;
	struct sockaddr_in caddr = { 0 };
	socklen_t len = sizeof caddr;
LB0:
	cfd = accept(sockfd, (struct sockaddr *)&caddr, &len);
	switch (fork()) {
	case -1:
		printf("[E] 无法处理Object %d\n", cfd);
		perror("fork失败\n");
		goto LB0;
	case 0:
		printf("[I] Clinet %d %s : %d 已连接\n", 
			cfd, inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
		break;
	default:
		goto LB0;
	}
	while (1) {
		char buf[1024] = { 0 };
		int len = read(cfd, buf, sizeof buf);
		if (len == 0 || len == -1) {
			printf("[I] %d 客户端断开\n", cfd);
			goto LB1;
		}
		printf("[I] %s.\n", buf);
		if (strcmp("exit\r\n", buf) == 0) {
			printf("[I] %d 客户端主动退出\n", cfd);
			write(cfd, "Good bye!\n", 10);
			goto LB1;
		}
	}
LB1:
	shutdown(cfd, SHUT_RDWR);
	close(cfd);
	printf("[I] %d exited\n", cfd);
	return 0;
}
