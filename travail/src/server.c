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

#define MAX_CONN 5

int handle_bind()
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, SERV_PORT, &hints, &result) != 0)
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
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

void handle_poll(struct pollfd* fds, int sfd,struct sockaddr cli,socklen_t len)
{
	while (1)
	{
		printf("will poll\n");
		int nfds = poll(fds, MAX_CONN, -1);
		printf("%d fd active\n", nfds);
		for (int i = 0; i < MAX_CONN; i++)
		{

			// IF listening socket gets a new request
			if (fds[i].fd == sfd && (fds[i].revents & POLLIN))
			{
				for (int i = 1; i < MAX_CONN; i++)
				{
					// if fd is available
					if (fds[i].fd == -1)
					{
						// handle accept
						if ((fds[i].fd = accept(sfd, (struct sockaddr *)&cli, &len)) < 0)
						{
							perror("accept()\n");
							exit(EXIT_FAILURE);
						}
						printf("New socket on fd(%d)\n", fds[i].fd); // listen 3, new 4
						fds[i].events = POLLIN;
						fds[i].revents = 0;
						break;
					}
				}
			}
			// if some other socket has an update
			else if (fds[i].fd != sfd && (fds[i].revents & POLLIN))
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
				printf("Received: %s", buff);
				// Sending message (ECHO)
				if (send(fds[i].fd, buff, strlen(buff), 0) <= 0)
				{
					break;
				}
				printf("Message sent!\n");
				fds[i].revents = 0;
			}
			// if some other socket closed
			else if (fds[i].fd != sfd && (fds[i].revents & POLLHUP))
			{
				// close
				close(fds[i].fd);

				fds[i].fd = -1;
				fds[i].events = 0;
				fds[i].revents = 0;
			}
		}
	}
}

int main()
{
	struct sockaddr cli ;
	memset(&cli, 0, sizeof(struct sockaddr));// fix because compiler throwed a warning because cli was not initialized
	int sfd;
	socklen_t len;
	sfd = handle_bind();
	if ((listen(sfd, SOMAXCONN)) != 0)
	{
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
	len = sizeof(cli);

	// polling logic
	struct pollfd fds[MAX_CONN] = {0}; // TODO : replace by linked list

	for (int i = 1; i < MAX_CONN; i++)
	{
		fds[i].fd = -1;
		fds[i].events = 0;
		fds[i].revents = 0;
	}
	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	handle_poll(fds,sfd,cli,len);

	close(sfd);
	return EXIT_SUCCESS;
}
