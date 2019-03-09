#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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

void *
showDownProgress(void *param) {
#define K 1024
	int **info = param;
	int le = *info[1];
	int v = 1;
	char korm = 'K';
	if (le > 10 * K * K) {
		v = K;
		korm = 'M';
	}
	while (1) {
		int old = *info[0];
		if (old >= le)
			break;
		sleep(1);
		if (*info[0] >= le)
			break;
		printf("%5.2lf %% \t %8d / %d %cB\t %d KB/s\n", 
			(double)*info[0] / le * 100,
			*info[0] / 1024 / v,
			le / 1024 / v, korm,
//			(*info[0] - old) / 1024 / v, korm);
			(*info[0] - old) / 1024);
	}
	return NULL;
}

int
download(int sfd, int nfd, int le, const char *name) {
#define BUFSIZE 102400
	char buf[BUFSIZE];
	int count = 0;
	int rl = 0;
	if (le <= 0)
		return -1;
/*
	int nfd = open(name, O_CREAT | O_WRONLY, 0664);
	if (nfd < 0) {
		printf("创建本地文件失败\n");
	}
*/
	int *info[2] = { &count, &le };
	pthread_t th = { 0 };
	pthread_create(&th, NULL, showDownProgress, &info);
	while (1) {
		rl = read(sfd, buf, le - count > BUFSIZE ? sizeof buf : le - count);
//		printf("%d %d\n", rl, count);
		if (rl == 0 || rl == -1) {
			close(nfd);
			return -1;
		}
		write(nfd, buf, rl);
		count += rl;
		if (count >= le)
			break;
	}
	close(nfd);
	return 0;
}

int
main(int argc, char *argv[]) {
	const char *sip = "b.javac.ga";
	if (argc == 2) {
		sip = argv[1];
	}
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
	if (connect(sockfd, (struct sockaddr *)&sa, (socklen_t)sizeof sa) != 0) {
		perror("[E] connect failed");
		return 1;
	}
	while (1) {
		char result[1024 * 8] = { 0 };
		int n = read(sockfd, result, sizeof result);
		if (n == 0 || n == -1) {
			perror("连接断开");
			break;
		}
		if (result[0] != '-')
			printf("%s\n", result);
		// 处理协议
		if (strcmp("Good bye!", result) == 0)
			break;
		// 服务器发送文件到本地
		int flength = 0;
		char fname[1024] = { 0 };
#define FILEINFO "-SENDFILE:%d:"
		if (sscanf(result, FILEINFO, &flength) == 1) {
			sprintf(fname, FILEINFO, flength);
			sprintf(fname, "%s", &result[strlen(fname)]);
			if (flength == 0) {
				printf("文件长度为 0, 取消任务\n");
				write(sockfd, "-Cancel", 8);
				char buf[1024];
				read(sockfd, buf, sizeof buf);
				printf("%s\n", buf);
				goto NEXTCMD;
			}
			int nfd = open(fname, O_WRONLY | O_CREAT, 0664);
			if (nfd < 0) {
				printf("创建本地文件失败!\n");
				write(sockfd, "NO", 3); //取消下载
			}
			long fle = lseek(nfd, SEEK_END, 0);
			printf("下载文件 %s, %d 字节, 从 %ld 开始\n", fname, flength, fle);
			// 发送确认讯号 + 断点续传
#define AC "-Accept:%ld#"
			char buf_ac[1024] = { 0 };
			sprintf(buf_ac, AC, fle);
			int status = write(sockfd, buf_ac, strlen(buf_ac) + 1);
			if (status == 0 || status == -1) {
				printf("下载失败\n");
				goto NEXTCMD;
			}
			// 接受flength个字节存储为文件fname
			if (download(sockfd, nfd, flength, fname) != 0) {
				printf("下载失败\n");
				goto NEXTCMD;
			}
			char buf[1024] = { 0 };
			read(sockfd, buf, sizeof buf);
			if (strcmp(buf, "-SENDOK") != 0) {
				printf("下载完成，但验证失败，请检查。\n");
				goto NEXTCMD;
			}
			printf("下载成功\n");
		}
		char buf[1024] = { 0 };
NEXTCMD:
		printf("===========================\n$ ");
		// 客户端命令以\n结束 例如"get /bin/ssh\n\0"
		if (fgets(buf, sizeof buf - 1, stdin) == NULL) { // ^D
			printf("^D\n");
			write(sockfd, "exit\n", 6);
		} else {
			write(sockfd, buf, strlen(buf) + 1);
		}
	}
	close(sockfd);
	return 0;
}
