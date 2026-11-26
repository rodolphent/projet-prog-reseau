#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "client_list.h"
#include "common.h"
#include "msg_struct.h"
#include "server_command.h"

static void accept_client(int server_fd, struct Client **clients)
{
	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	int fd = accept(server_fd, &addr, &len);
	die(fd, "accept()");
	printf("New socket on fd(%d)\n", fd);
	client_add(clients, fd, &addr, len);
}

static void handle_client_input(int fd, struct Client **clients)
{
	char buffer[MSG_LEN];
	struct message msgstruct;

	buffer[0] = '\0';
	memset(buffer, 0, sizeof(buffer));
	memset(&msgstruct, 0, sizeof(struct message));

	//getting metadata
	if (recv(fd, &msgstruct, sizeof(struct message), 0) <= 0)
	{
		printf("closed \n");
		client_remove(clients, fd);
		return;
	}

	//look for the right client in the linked list q
	struct Client *client = *clients;
	while (client != NULL && client->fd != fd)
	{
		client = client->next;
	}
	if (client == NULL)
	{
		fprintf(stderr, "Client not found for fd(%d)\n", fd);
		client_remove(clients, fd);
		return;
	}
	if (client != NULL)
	{
		client->msgstruct = msgstruct;
		client->msg_content = buffer;

	}
	if (msgstruct.pld_len != 0) die(recv(fd, buffer, msgstruct.pld_len,0), "recv");

	printf("Received data: %s\n", msgstruct.nick_sender);
	printf("Received payload: %s\n", buffer);

	handle_command(clients,client);
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

void handle_poll(int server_fd, struct Client **clients)
{
	while (1)
	{
		int n = client_count(*clients) + 1;
		struct pollfd fds[n];
		fds[0].fd = server_fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		int i = 1;
		for (struct Client *c = *clients; c != NULL; c = c->next, i++)
		{
			fds[i].fd = c->fd;
			fds[i].events = POLLIN;
			fds[i].revents = 0;
		}

		printf("will poll\n");
		int nfds = poll(fds, n, -1);
		die(nfds, "poll()");
		printf("%d fd active\n", nfds);

		if (fds[0].revents & POLLIN)
		{
			accept_client(server_fd, clients);
		}
		for (i = 1; i < n; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				handle_client_input(fds[i].fd, clients);
			}
			else if (fds[i].revents & POLLHUP)
			{
				client_remove(clients, fds[i].fd);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	char *address = SERV_ADDR;
	char *port = SERV_PORT;
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

	sfd = handle_bind(address, port);
	if ((listen(sfd, SOMAXCONN)) != 0)
	{
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}

	struct Client *clients = NULL; 

	handle_poll(sfd, &clients);

	client_free_all(&clients);
	close(sfd);
	return EXIT_SUCCESS;
}
