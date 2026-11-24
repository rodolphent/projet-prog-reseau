#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>

#include "common.h"

static void die(int ret_value, const char *msg)
{
	if (ret_value < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

#define MAX_CONN 5

static void accept_client(struct pollfd *fds, int server_fd,
						  struct sockaddr *client_address,
						  socklen_t *client_address_len)
{
	for (int i = 1; i < MAX_CONN; i++)
	{
		if (fds[i].fd == -1)
		{
			if ((fds[i].fd = accept(server_fd, client_address,
									client_address_len)) < 0)
			{
				die(-1, "accept()\n");
				exit(EXIT_FAILURE);
			}
			printf("New socket on fd(%d)\n", fds[i].fd);
			fds[i].events = POLLIN;
			fds[i].revents = 0;
			break;
		}
	}
}

static int handle_client_input(struct pollfd *client)
{
	char buffer[MSG_LEN];

	memset(buffer, 0, sizeof(buffer));
	if (recv(client->fd, buffer, MSG_LEN, 0) <= 0)
	{
		printf("closed \n");
		close(client->fd);
		client->fd = -1;
		client->events = 0;
	}
	printf("Received: %s", buffer);
	if (send(client->fd, buffer, strlen(buffer), 0) <= 0)
	{
		return 0;
	}
	printf("Message sent!\n");
	client->revents = 0;
	return 1;
}

static void close_client(struct pollfd *client)
{
	close(client->fd);
	client->fd = -1;
	client->events = 0;
	client->revents = 0;
}

int handle_bind(char *address, char *port)
{
	struct addrinfo hints, *result, *rp;
	int sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(address, port, &hints, &result) != 0)
	{
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype,
					 rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}
		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); // permet de réutiliser le port si le serveur est relancé rapidement

		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		die(-1, "Could not bind\n");
	}
	freeaddrinfo(result);
	return sfd;
}

int handle_command(char *msg, int l)
{

	int i = 1;
	while ((i < l) && (!isspace(msg[i])) && (msg[i] != '\0'))
	{

		i++;
	}

	char command[i + 1];
	memcpy(command, msg, i);
	command[i] = '\0';

	printf("command is : %s\n", command);

	if (strcmp("/quit", command) == 0)
	{
		printf("quitting\n");
		exit(EXIT_SUCCESS);
	}
	return 1;
}

void handle_poll(struct pollfd *fds, int sfd, struct sockaddr cli, socklen_t len)
{
	while (1)
	{
		printf("will poll\n");
		int nfds = poll(fds, MAX_CONN, -1);
		printf("%d fd active\n", nfds);

		for (int i = 0; i < MAX_CONN; i++)
		{
			if (fds[i].fd == server_fd && (fds[i].revents & POLLIN))
			{
				accept_client(fds, server_fd, &client_address,
							  &client_address_len);
			}
			else if (fds[i].fd != server_fd && (fds[i].revents & POLLIN))
			{
				// read data

				char buff[MSG_LEN];

				// Cleaning memory
				memset(buff, 0, MSG_LEN);
				// Receiving message
				if (recv(fds[i].fd, buff, MSG_LEN, 0) <= 0)
				{
					printf("closed \n");
					close(fds[i].fd);
					fds[i].fd = -1;
					fds[i].events = 0;
				}
				else
				{
					// check if this is a command
					if (buff[0] == '/')
					{
						handle_command(buff, sizeof(buff));
					}
				}
				printf("Received: %s", buff);
				// Sending message (ECHO)

				if (send(fds[i].fd, buff, strlen(buff), 0) <= 0)
				{
					break;
				}
			}
			else if (fds[i].fd != server_fd && (fds[i].revents & POLLHUP))
			{
				close_client(&fds[i]);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	char *address = SERV_ADDR;
	char *port = SERV_PORT;
	struct sockaddr client_address;
	struct sockaddr cli;
	memset(&cli, 0, sizeof(struct sockaddr)); // fix because compiler throwed a warning because cli was not initialized
	int sfd;

	if (argc != 3)
	{
		printf("Default address and port will be used : %s:%s\n", address, port);
	}
	else
	{
		address = argv[1];
		port = argv[2];
	}

	memset(&client_address, 0, sizeof(struct sockaddr));

	socklen_t len;
	sfd = handle_bind(address, port);
	if ((listen(sfd, SOMAXCONN)) != 0)
	{
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
	len = sizeof(client_address);

	struct pollfd fds[MAX_CONN] = {0};

	for (int i = 1; i < MAX_CONN; i++)
	{
		fds[i].fd = -1;
		fds[i].events = 0;
		fds[i].revents = 0;
	}
	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	handle_poll(fds, sfd, client_address, len);

	close(sfd);
	return EXIT_SUCCESS;
}
