#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include <sys/socket.h>
#include "msg_struct.h"

struct Client
{
	int fd;
	struct sockaddr address;
	socklen_t address_len;
	struct Client *next;
	char nickname[NICK_LEN];
	struct message msgstruct;
	char* msg_content; // do malloc when using
	int is_waiting_message;
	// add message struct
	//add payload struct NULL
	// when i receive a message, i cast it in message struct
	// when i recevie a message, and message struct is not null i cast it in payload
	// once payload is not NULL and is treated, clean everything
};

void client_add(struct Client **head, int fd, struct sockaddr *addr,
				socklen_t len);
void client_remove(struct Client **head, int fd);
void client_free_all(struct Client **head);
int client_count(struct Client *head);

#endif
