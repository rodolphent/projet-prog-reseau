#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include <sys/socket.h>
#include <time.h>
#include "msg_struct.h"

struct Client
{
	int fd;
	struct sockaddr address;
	socklen_t address_len;
	time_t connected_at; 
	struct Client *next;
	char nickname[NICK_LEN];
	struct message msgstruct;
	char* msg_content; // do malloc when using
	int is_waiting_message;
	char channel[CHANNEL_LEN];
};

void client_add(struct Client **head, int fd, struct sockaddr *addr,
				socklen_t len);
void client_remove(struct Client **head, int fd);
void client_free_all(struct Client **head);
int client_count(struct Client *head);

int notify_channel(struct Client *head, const char *channel, const char *message, struct Client *sender);
int leave_channel(struct Client *head, struct Client *client);

int check_channel_name(struct Client **clients, const char *name, char *content, size_t size);

#endif
