#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int
main(int argc, char *argv[]) {
	// int socket(int domain, int type, int protocol);
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		perror("socket()");
		return 1;
	}
	return 0;
}
