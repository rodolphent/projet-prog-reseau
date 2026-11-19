#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

void die(int ret_value, const char *msg)
{
	if (ret_value < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

void echo_client(int sockfd)
{
	char buff[MSG_LEN];
	int n;
	struct pollfd fds[2];
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1)
	{
		die(poll(fds, 2, -1), "poll");
		// Cleaning memory
		memset(buff, 0, sizeof(buff));
		if (fds[0].revents & (POLLIN | POLLHUP | POLLERR))
		{
			n = read(sockfd, buff, sizeof(buff) - 1);
			if (n < 0)
			{
				perror("read socket");
				break;
			}
			if (n == 0)
			{
				printf("Server closed the connection\n");
				break;
			}
			printf("Received: %s", buff);
		}
		if (fds[1].revents & (POLLIN | POLLHUP | POLLERR))
		{
			n = read(STDIN_FILENO, buff, sizeof(buff) - 1);
			if (n < 0)
			{
				perror("read stdin");
				break;
			}
			if (n == 0)
			{
				printf("stdin closed\n");
				break;
			}
			if (send(sockfd, buff, n, 0) <= 0)
			{
				break;
			}
			printf("Message sent!\n");
		}
	}
}

int handle_connect()
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	die(getaddrinfo(SERV_ADDR, SERV_PORT, &hints, &result) == 0, "getaddrinfo");
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			break;
		}
		close(sfd);
	}
	die(rp != NULL, "connect");
	printf("Client connected !\n");
	freeaddrinfo(result);
	return sfd;
}

int main()
{
	int sfd;
	sfd = handle_connect();
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}
