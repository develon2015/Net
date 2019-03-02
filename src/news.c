/**
 * 简易文件传输 服务端
 * 
 */
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>

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
	const char *info = "Welcome to develon's house! 欢迎光临寒舍";
	write(cfd, info, strlen(info) + 1);
	while (1) {
		char buf[1024] = { 0 };
		int len = read(cfd, buf, sizeof buf);
		if (len == 0 || len == -1) {
			printf("[I] %d 客户端断开\n", cfd);
			goto LB1;
		}
		printf("[I] %s", buf);
#define ERROR 0
#define HELP 1
#define EXIT 2
#define PWD 3
#define LS 4
#define CD 5
#define GET 6
#define PUT 7
		int cat = ERROR;
		if (strcmp("exit\n", buf) == 0) {
			cat = EXIT;
			goto CAT;
		}
		if (strcmp("help\n", buf) == 0) {
			cat = HELP;
			goto CAT;
		}
		if (strcmp("pwd\n", buf) == 0) {
			cat = PWD;
			goto CAT;
		}
		char tdir[1024] = { 0 };
		if (sscanf(buf, "ls %s\n", tdir) == 1) {
			cat = LS;
			goto CAT;
		}
		if (strcmp(buf, "cd\n") == 0 || strcmp(buf, "cd \n") == 0) {
			sprintf(tdir, "%s", getenv("HOME"));
			cat = CD;
			goto CAT;
		}
		if (sscanf(buf, "cd %s\n", tdir) == 1) {
			cat = CD;
			goto CAT;
		}
		if (strcmp("ls\n", buf) == 0) {
			cat = LS;
			sprintf(tdir, ".");
			goto CAT;
		}
		if (sscanf(buf, "get %s\n", tdir) == 1) {
			cat = GET;
			goto CAT;
		}
		if (sscanf(buf, "put %s\n", tdir) == 1) {
			cat = PUT;
			goto CAT;
		}
CAT:
		switch (cat) {
		case ERROR:
#define ES "Unknow instruction"
			write(cfd, ES, strlen(ES) + 1);
			break;
		case EXIT:
			printf("[I] %d 客户端主动退出\n", cfd);
			write(cfd, "Good bye!", 10);
			goto LB1;
			break;
		case HELP:
			write(cfd, "暂无帮助信息", 19);
			break;
		case PWD:
			{
				char *buf = getcwd(NULL, 0);
				write(cfd, buf, strlen(buf) + 1);
				free(buf);
			}
			break;
		case LS:
			{
				DIR *dir = opendir(tdir);
				if (dir == NULL) {
					char *es = strerror(errno);
					write(cfd, es, strlen(es) + 1);
					break;
				}
				char list[1024 * 20] = { 0 };
				struct dirent *pd;
				while ((pd = readdir(dir)) != NULL) {
					// 隐藏.开头的文件和目录
					if (pd->d_name[0] == '.')
						continue;
				strcat(list, pd->d_name);
				strcat(list, "\n");
				}
				list[strlen(list) - 1] = '\0';
				write(cfd, list, strlen(list) + 1);
			}
			break;
		case CD:
			{
				if (chdir(tdir) != 0) {
					char *es = strerror(errno);
					write(cfd, es, strlen(es) + 1);
					break;
				}
				write(cfd, "OK", 3);
			} 
			break;
		case GET:
			{
				struct stat statbuf = { 0 };
				if (stat(tdir, &statbuf) != 0) {
					char *es = strerror(errno);
					write(cfd, es, strlen(es) + 1);
					break;
				}
				if ((statbuf.st_mode & S_IFMT)!= S_IFREG) {
#define ESNFILE "不是常规文件"
					write(cfd, ESNFILE, strlen(ESNFILE) + 1);
					break;
				}
				// 
				int fd = open(tdir, O_RDONLY);
				if (fd < 0) {
					char *es = strerror(errno);
					write(cfd, es, strlen(es) + 1);
					break;
				}
#define SENDFILE "-SENDFILE:%d:%s="
				char buf[102400] = { 0 };
				sprintf(buf, SENDFILE, (int)statbuf.st_size, basename(tdir));
				write(cfd, buf, strlen(buf) + 1);
				// 确认发送
				int status = read(cfd, buf, sizeof buf);
				if (status == 0 || status == -1) {
					printf("[E] 连接断开\n");
					close(fd);
					break;
				}
				if (strcmp("-Accept", buf) == 0) {
					// 对方已开始接受文件
					while (1) {
						int rl = read(fd, buf, sizeof buf);
						if (rl == 0) { // EOF
							printf("[I] 文件 %s 传输完成\n", tdir);
							break;
						}
						if (rl == -1) {
							perror("[E] 文件传输错误");
							break;
						}
						write(cfd, buf, rl);
						printf("[I] %d byte已发送\n", rl);
					}
					close(fd);
					// 确认成功讯号
					write(cfd, "-SENDOK", 7);
				} else {
#define CANCEL "服务器取消发送"
					printf("[I] 对方取消接受\n");
					write(cfd, CANCEL, strlen(CANCEL) + 1);
					close(fd);
					break;
				}
			}
			break;
		case PUT:
			write(cfd, "暂不支持上传文件", 25);
			break;
		}
	}
LB1:
	shutdown(cfd, SHUT_RDWR);
	close(cfd);
	printf("[I] %d exited\n", cfd);
	return 0;
}
