#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <common.h>

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
	c->connected_at = time(NULL);
	c->next = *head;
	c->is_waiting_message = 0;
	c->file_status = FILE_NONE;
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

		//notify other users
		if (to_free->channel[0] != '\0')
		{
			char old[CHANNEL_LEN];
			snprintf(old, sizeof(old), "%s", to_free->channel);
			leave_channel(*head, to_free);
			char msg[MSG_LEN];
			snprintf(msg, sizeof(msg), "%s has quit the channel", to_free->nickname);
			notify_channel(*head, old, msg, to_free);
		}


		close(to_free->fd);
		free(to_free);
		//printf("Client on fd(%d) removed\n", fd);
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


int notify_channel(struct Client *head, const char *channel, const char *message, struct Client *sender)
{
	int count = 0;
	if (channel[0] == '\0')
		return 0; // pas de salon : ne pas notifier les clients hors salon

	struct message msg;
	memset(&msg, 0, sizeof(msg));
	snprintf(msg.nick_sender, sizeof(msg.nick_sender), "%s", "Server");
	msg.type = MULTICAST_SEND;
	msg.pld_len = strlen(message);
	snprintf(msg.infos, sizeof(msg.infos), "%s", channel);

	for (struct Client *c = head; c != NULL; c = c->next)
	{
		//check if the client is in the channel and is not the sender (sender may be NULL)
		if (strcmp(c->channel, channel) == 0 && (sender == NULL || c->fd != sender->fd))
		{
			send(c->fd, &msg, sizeof(msg), 0);
			send(c->fd, message, msg.pld_len, 0);
			count++;
		}
	}
	return count;
}


int leave_channel(struct Client *head, struct Client *client)
{
    char old[CHANNEL_LEN];
    snprintf(old, sizeof(old), "%s", client->channel);
    client->channel[0] = '\0';

    int remaining = 0;
    for (struct Client *c = head; c != NULL; c = c->next)
        if (c->channel[0] != '\0' && strcmp(c->channel, old) == 0)
            remaining++;
    return remaining;      //0 if channel is empty, >0 otherwise
}


int channel_exists(struct Client *head, const char *name)
{
    for (struct Client *c = head; c != NULL; c = c->next)
        if (c->channel[0] != '\0' && strcmp(c->channel, name) == 0)
            return 1;
    return 0;
}

int check_channel_name(struct Client **clients, const char *name, char *content, size_t size)
{
    size_t len = strlen(name);
    if (len == 0 || len >= CHANNEL_LEN)
        { snprintf(content, size, "Invalid channel name : 1 to %d characters\n", CHANNEL_LEN - 1); return 1; }
    for (size_t i = 0; i < len; i++)
        if (!isalnum((unsigned char)name[i]))
            { snprintf(content, size, "Invalid channel name : letters and digits only\n"); return 1; }
    if (channel_exists(*clients, name))
        { snprintf(content, size, "Channel %s already exists\n", name); return 1; }
    return 0;
}