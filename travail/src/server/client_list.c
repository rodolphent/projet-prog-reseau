#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client_list.h"

void client_add(struct Client **head, int fd, struct sockaddr *addr,
					   socklen_t len)
{
	struct Client *c = calloc(1,sizeof(*c));
	if (!c)
	{
		perror("malloc()");
		exit(EXIT_FAILURE);
	}
	c->fd = fd;
	c->address = *addr;
	c->address_len = len;
	c->next = *head; 
	c->is_waiting_message = 0;
	*head = c;
}

void client_remove(struct Client **head, int fd)
{
	struct Client **cur = head;
	while (*cur && (*cur)->fd != fd)
	{
		cur = &(*cur)->next;
	}
	if (*cur)
	{
		struct Client *to_free = *cur;
		*cur = to_free->next; 
		close(to_free->fd);
		free(to_free);
		printf("Client on fd(%d) removed\n", fd);
	}
}

void client_free_all(struct Client **head)
{
	while (*head)
	{
		client_remove(head, (*head)->fd);
	}
}

int client_count(struct Client *head)
{
	int n = 0;
	for (struct Client *c = head; c != NULL; c = c->next)
	{
		n++;
	}
	return n;
}
