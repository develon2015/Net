#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void
resolveIP(const char *domain, int type) {
	printf("%s: (IPv%d)\n", domain, type == AF_INET ? 4 : 6);
	struct hostent *ent = gethostbyname2(domain, type);
	if (ent == NULL) {
		//herror("[E]");
		printf("\t%s\n", hstrerror(h_errno));
		return;
	}
	int i = 0;
	for (; ent->h_addr_list[i] != NULL; i++) {
		void *ip= ent->h_addr_list[i];
		char buf[1024];
		inet_ntop(type, ip, buf, sizeof buf);
		printf("\t%s\n", buf);

	}
}
int
main(int argc, char *argv[]) {
	resolveIP(argv[1], AF_INET);
	resolveIP(argv[1], AF_INET6);
	return 0;
}

/**
develon@desktop:~/git/Net$ bin/gethostbyname  javac.ga
javac.ga: (IPv4)
        104.18.45.94
        104.18.44.94
javac.ga: (IPv6)
        2606:4700:30::6812:2d5e
        2606:4700:30::6812:2c5e

*/
