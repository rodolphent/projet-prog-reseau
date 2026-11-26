#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "command.h"
#include "common.h"
#include "msg_struct.h"

static int receive_all(int sockfd, void *buffer, size_t length)
{
	size_t received = 0;
	while (received < length)
	{
		ssize_t n = recv(sockfd, (char *)buffer + received, length - received, 0);
		if (n < 0)
		{
			perror("receive from server");
			return 0;
		}
		if (n == 0)
		{
			printf("Server closed the connection\n");
			return 0;
		}
		received += (size_t)n;
	}
	return 1;
}

int handle_server_input(int sockfd, char *nickname, char *buffer)
{
	struct message response;
	if (!receive_all(sockfd, &response, sizeof(response)))
	{
		return 0;
	}

	if (response.pld_len < 0 || response.pld_len >= MSG_LEN)
	{
		fprintf(stderr, "Invalid payload length received from server\n");
		return 0;
	}

	if (!receive_all(sockfd, buffer, (size_t)response.pld_len))
	{
		return 0;
	}
	buffer[response.pld_len] = '\0';

	if (response.type == NICKNAME_NEW)
	{
		if (strcmp(nickname, response.infos) != 0){
			printf("Nickname changed from %s to %s\n", nickname, response.infos);
		}else {
			printf("Invalid nickname : %s\n", response.infos);
		}
		snprintf(nickname, NICK_LEN, "%s", response.infos);
	}

	if (response.type == NICKNAME_LIST)
	{
		//nothing to do, will already print
		
	}

	if (response.type == BROADCAST_SEND)
	{
		printf("[%s] : %s\n", response.nick_sender, buffer);
		return 1; // :/
	}

	if ((response.type == UNICAST_SEND) && (strcmp(response.nick_sender, "Server") != 0))
	{
		printf("[%s] whispers to you : %s\n", response.nick_sender, buffer);
		return 1; 
	}

	printf("[Server] : %s", buffer);
	return 1;
}

void echo_client(int sockfd, char *nickname)
{
	char buffer[MSG_LEN];
	struct pollfd fds[2];
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1)
	{
		die(poll(fds, 2, -1), "poll");
		memset(buffer, 0, sizeof(buffer));

		if (fds[0].revents & (POLLIN | POLLHUP | POLLERR))
		{
			if (!handle_server_input(sockfd, nickname, buffer))
			{
				break;
			}
		}

		if (fds[1].revents & (POLLIN | POLLHUP | POLLERR))
		{
			if (!handle_stdin_input(sockfd, nickname, buffer))
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

	int status = getaddrinfo(address, port, &hints, &result);
	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

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

	if (rp == NULL)
	{
		die(-1, "Could not connect to server\n");
	}
	printf("Client connected !\n");
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char *argv[])
{
	int sfd;
	char nickname[NICK_LEN] = "";

	if (argc < 3)
	{
		printf("Default address and port will be used : %s:%s\n", SERV_ADDR, SERV_PORT);
		sfd = handle_connect(SERV_ADDR, SERV_PORT);
	}
	else
	{
		sfd = handle_connect(argv[1], argv[2]);
	}

	echo_client(sfd, nickname);
	close(sfd);
	return EXIT_SUCCESS;
}
