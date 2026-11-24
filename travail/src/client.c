#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

static int handle_server_input(int sockfd, char *buffer)
{
	int n = read(sockfd, buffer, MSG_LEN - 1);
	if (n < 0)
	{
		perror("read socket");
		return 0;
	}
	if (n == 0)
	{
		printf("Server closed the connection\n");
		return 0;
	}

	printf("Received: %s", buffer);
	return 1;
}

static int handle_stdin_input(int sockfd, char *buffer)
{
	int n = read(STDIN_FILENO, buffer, MSG_LEN - 1);
	if (n < 0)
	{
		perror("read stdin");
		return 0;
	}
	if (n == 0)
	{
		printf("stdin closed\n");
		return 0;
	}
	if (send(sockfd, buffer, n, 0) <= 0)
	{
		return 0;
	}

	printf("Message sent!\n");
	return 1;
}

void die(int ret_value, const char *msg)
{
	if (ret_value != 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

void echo_client(int sockfd)
{
	char buffer[MSG_LEN];
	struct pollfd fds[2];
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1)
	{
		die(poll(fds, 2, -1) < 0, "poll");
		memset(buffer, 0, sizeof(buffer));

		if (fds[0].revents & (POLLIN | POLLHUP | POLLERR))
		{
			if (!handle_server_input(sockfd, buffer))
			{
				break;
			}
		}

		if (fds[1].revents & (POLLIN | POLLHUP | POLLERR))
		{
			if (!handle_stdin_input(sockfd, buffer))
			{
				break;
			}
		}
	}
}

int handle_connect(char *address, char *port)
{
	struct addrinfo hints, *result, *rp;
	int sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	die(getaddrinfo(address, port, &hints, &result), "getaddrinfo");

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

	die(rp == NULL, "connect");
	printf("Client connected !\n");
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char *argv[])
{
	int sfd;

	if (argc != 3)
	{
		printf("Default address and port will be used : %s:%s\n", SERV_ADDR, SERV_PORT);
		sfd = handle_connect(SERV_ADDR, SERV_PORT);
	}
	else
	{
		sfd = handle_connect(argv[1], argv[2]);
	}

	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}
