#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>

int
resolveIPv4(const char *domain, struct in_addr *sin_addr) {
	struct hostent *ent = gethostbyname2(domain, AF_INET);
	if (ent == NULL) {
		printf("[E] can't resolve the IP: %s\n", hstrerror(h_errno));
		return -1;
	}
	*sin_addr = *(struct in_addr *)ent->h_addr;
	return 0;
}

int
main(int argc, char *argv[]) {
	if (argc == 1) {
	}
	const char *sip = "b.javac.ga";
	sip = argv[1];
	struct sockaddr_in sa = { 0 };
	sa.sin_family = AF_INET;
	sa.sin_port = htons(6621);
	// domain 2 ip
	if (resolveIPv4(sip, &sa.sin_addr) != 0) {
		return 1;
	}
	printf("[I] %s -> %s\n", sip, inet_ntoa(sa.sin_addr));
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	int cf = connect(sockfd, (struct sockaddr *)&sa, (socklen_t)sizeof sa) ;
	if (cf < 0) {
		perror("[E] connect failed");
		return 1;
	}
	return 0;
}
