#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"
#include "server_command.h"
#include <ctype.h>

int check_nickname(struct Client **clients, char *nickname, char *content, size_t content_size)
{

	struct Client *client = *clients;
	while (client != NULL)
	{
		if (strcmp(client->nickname, nickname) == 0)
		{
			snprintf(content, content_size, "Invalid username : already taken\n");
			return 1;
		}
		client = client->next;
	}

	if (strlen(nickname) < 4)
	{
		snprintf(content, content_size, "Invalid username : please provide at least 4 characters\n");
		return 1;
	}

	if (strlen(nickname) >= NICK_LEN)
	{
		snprintf(content, content_size, "Invalid username : above max (%d) characters\n", NICK_LEN - 1);
		return 1;
	}
	for (size_t i = 0; i < strlen(nickname); i++)
	{
		if (!isalnum((unsigned char)nickname[i]) && nickname[i] != '_')
		{
			snprintf(content, content_size, "Invalid username : illegal character at pos %d\n", (int)i);
			return 1;
		}
	}

	snprintf(content, content_size, "Welcome to the server %s !\n", nickname);
	return 0;
}

struct Client *find_client_by_nickname(struct Client **clients, const char *nickname)
{
	struct Client *client = *clients;
	while (client != NULL)
	{
		if (strcmp(client->nickname, nickname) == 0)
		{
			return client;
		}
		client = client->next;
	}
	return NULL; // Not found
}

void not_found_response(struct Client *client)
{
	char content[MSG_LEN];
	struct message response;
	memset(&response, 0, sizeof(response));

	snprintf(content, sizeof(content), "User %s does not exist\n", client->msgstruct.infos);

	snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
	response.type = UNICAST_SEND;
	response.pld_len = strlen(content);

	send(client->fd, &response, sizeof(response), 0);
	send(client->fd, content, strlen(content), 0);
}

int handle_command(struct Client **clients, struct Client *client)
{

	// TODO : handle quit
	if (client->msgstruct.type == 9999)
	{
		printf("quitting\n");
		return 0;
	}

	if (client->msgstruct.type == NICKNAME_NEW)
	{
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		// check nickname validity
		int nick_status = check_nickname(clients, client->msgstruct.infos, content, sizeof(content));
		if (nick_status != 0)
		{
			printf("invalid nickname for fd(%d) : %s\n", client->fd, client->msgstruct.infos);
			snprintf(response.infos, sizeof(response.infos), "%s", client->nickname);
		}
		else
		{
			printf("nickname changed for fd(%d) from %s to %s\n", client->fd, client->nickname, client->msgstruct.infos);
			snprintf(client->nickname, NICK_LEN, "%s", client->msgstruct.infos);
			snprintf(response.infos, sizeof(response.infos), "%s", client->nickname);
		}

		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = NICKNAME_NEW;
		response.pld_len = strlen(content);

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
	}

	if (client->msgstruct.type == NICKNAME_LIST)
	{
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		snprintf(content, sizeof(content), "Connected clients :\n");
		for (struct Client *c = *clients; c != NULL; c = c->next)
		{
			if (strlen(c->nickname) > 0)
			{
				strncat(content, c->nickname, sizeof(content) - strlen(content) - 1);
				strncat(content, "\n", sizeof(content) - strlen(content) - 1);
			}
		}

		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = NICKNAME_LIST;
		response.pld_len = strlen(content);

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
	}

	if (client->msgstruct.type == ECHO_SEND)
	{
		// TODO : echo things

		send(client->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
		send(client->fd, client->msg_content, strlen(client->msg_content), 0);

		printf("Message sent!\n");
	}

	if (client->msgstruct.type == NICKNAME_INFOS)
	{

		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		snprintf(content, sizeof(content), "User %s does not exist\n", client->msgstruct.infos);
		for (struct Client *c = *clients; c != NULL; c = c->next)
		{
			if (c->nickname[0] != '\0' && strcmp(c->nickname, client->msgstruct.infos) == 0)
			{
				struct sockaddr_in *addr = (struct sockaddr_in *)&c->address;
				char date[32];
				strftime(date, sizeof(date), "%Y/%m/%d@%H:%M", localtime(&c->connected_at));
				snprintf(content, sizeof(content),
						 "%s connected since %s with IP address %s and port number %d\n",
						 c->nickname, date, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
				break;
			}
		}

		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = NICKNAME_INFOS;
		response.pld_len = strlen(content);

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
	}

	if (client->msgstruct.type == BROADCAST_SEND)
	{

		for (struct Client *c = *clients; c != NULL; c = c->next)
		{
			if (c->fd != client->fd)
			{
				send(c->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
				send(c->fd, client->msg_content, strlen(client->msg_content), 0);
			}
		}

		printf("Message broadcasted : %s\n", client->msg_content);
	}

	if (client->msgstruct.type == UNICAST_SEND)
	{
		int found = 0;
		for (struct Client *c = *clients; c != NULL; c = c->next)
		{
			if (c->nickname[0] != '\0' && strcmp(c->nickname, client->msgstruct.infos) == 0)
			{
				send(c->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
				send(c->fd, client->msg_content, strlen(client->msg_content), 0);
				found = 1;
				break;
			}
		}
		if (!found)
		{
			char content[MSG_LEN];
			struct message response;
			memset(&response, 0, sizeof(response));

			snprintf(content, sizeof(content), "User %s does not exist\n", client->msgstruct.infos);

			snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
			response.type = UNICAST_SEND;
			response.pld_len = strlen(content);

			send(client->fd, &response, sizeof(response), 0);
			send(client->fd, content, strlen(content), 0);
		}
	}

	if (client->msgstruct.type == FILE_REQUEST)
	{
		struct Client *target_client;
		target_client = find_client_by_nickname(clients, client->msgstruct.infos);
		if (!target_client)
		{
			not_found_response(client);
		}
	}

	return 1;
}
