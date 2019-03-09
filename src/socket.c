#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void
handle(int fd) {
	{
		char buf[1024] = { 0 };
		int le = read(fd, buf, sizeof buf);
		if (le == 0) {
			printf("EOF\n");
			return ;
		}
		if (le == -1) {
			perror("read");
			return ;
		}
		printf("%s", buf);
	}
	const char *info = "HTTP/1.1 200 OK\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s";
	char buf[1024] = { 0 };
	const char *data = "Hello";
	sprintf(buf, info, strlen(data), data);
	write(fd, buf, strlen(buf) + 1);
}

int
main(int argc, char *argv[]) {
	// int socket(int domain, int type, int protocol);
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		perror("socket()");
		return 1;
	}
	struct sockaddr_in la = { 0 };
	la.sin_family = AF_INET;
	la.sin_port = htons(80);
	if (bind(sfd, (struct sockaddr *)&la, sizeof la) != 0) {
		perror("bind");
		return 1;
	}
	listen(sfd, 10);
	while (1) {
		struct sockaddr_in ca = { 0 };
		socklen_t le = sizeof ca;
		int cf = accept(sfd, (struct sockaddr *)&ca, &le);
		printf("[I] %s:%d connected\n", inet_ntoa(ca.sin_addr), ntohs(ca.sin_port));
		handle(cf);
		close(cf);
	}
	return 0;
}
