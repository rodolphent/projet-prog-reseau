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
#include "msg_struct.h"

void die(int ret_value, const char *msg)
{
	if (ret_value < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

int handle_server_input(int sockfd, char *buffer)
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

int handle_stdin_input(int sockfd, char* nickname, char *buffer)
{
	int n = read(STDIN_FILENO, buffer, MSG_LEN - 1);
	char *msg_type, *msg_infos;
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

	msg_type, msg_infos = get_msg_type_and_infos(buffer);
	struct message payload;
	payload.pld_len = n;
	payload.type = msg_type;
	memcpy(payload.nick_sender, nickname, NICK_LEN);
	payload.type = ECHO_SEND;
	memcpy(payload.infos, msg_infos, INFOS_LEN);

	if (send(sockfd, buffer, n, 0) <= 0)
	{
		return 0;
	}

	printf("Message sent!\n");
	return 1;
}

void echo_client(int sockfd, char* nickname)
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
			if (!handle_server_input(sockfd, buffer))
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

char *get_msg_type_and_infos(char *buffer)
{
	char *whitespace = " \t\n";
	char *msg_type = strtok(buffer, whitespace);
	char *msg_infos = strtok(NULL, "\n");
	return msg_type, msg_infos;
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
	char *nickname = "";

	if (argc < 3)
	{
		printf("Default address and port will be used : %s:%s\n", SERV_ADDR, SERV_PORT);
		sfd = handle_connect(SERV_ADDR, SERV_PORT);
	}
	else if (argc < 4){
		nickname = argv[3];
	}
	else
	{
		sfd = handle_connect(argv[1], argv[2]);
	}

	echo_client(sfd, nickname);
	close(sfd);
	return EXIT_SUCCESS;
}
