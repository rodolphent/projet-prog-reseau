#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"
#include "server_command.h"
#include "utils.h"
#include <ctype.h>

int check_nickname(struct Client **clients, char *nickname, char *content, size_t content_size)
{

	if (nickname[0] == '\0')
	{
		snprintf(content, content_size, "Invalid username : empty\n");
		return 1;
	}

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

	if (strlen(nickname) >= NICK_LEN)
	{
		snprintf(content, content_size, "Invalid username : above max (%d) characters\n", NICK_LEN - 1);
		return 1;
	}
	for (size_t i = 0; i < strlen(nickname); i++)
	{
		if (!isalnum((unsigned char)nickname[i]))
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

	if (client->msgstruct.type == NICKNAME_NEW)
	{
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		// check nickname validity
		int nick_status = check_nickname(clients, client->msgstruct.infos, content, sizeof(content));
		if (nick_status != 0)
		{
			printf("[ERROR] : fd(%d) Invalid nickname : %s\n", client->fd, client->msgstruct.infos);
			snprintf(response.infos, sizeof(response.infos), "%s", client->nickname);
		}
		else
		{
			printf("[INFO] : fd(%d) Nickname changed from %s to %s\n", client->fd, client->nickname, client->msgstruct.infos);
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
		memset(&response,0,sizeof(response));
		printf("[INFO] : fd(%d) Listing clients\n", client->fd);
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

	//workaround ; was echo before and now supports rooms
	if (client->msgstruct.type == MULTICAST_SEND) {
		
		if (client->channel[0] != '\0') {
			snprintf(client->msgstruct.infos, sizeof(client->msgstruct.infos), "%s", client->channel);
			//sends the message to all members in the room 
			for (struct Client *c = *clients; c != NULL; c = c->next)
			{
				if (strcmp(c->channel, client->channel) == 0 && c->fd != client->fd)
				{
					
					send(c->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
					send(c->fd, client->msg_content, strlen(client->msg_content), 0);
				}
			}

			return 1;
		} else {
			//default echo 
			printf("[INFO] : echo from fd(%d) : %s\n", client->fd, client->msg_content);
			send(client->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
			send(client->fd, client->msg_content, strlen(client->msg_content), 0) ;
	
		}
		
	}

	if (client->msgstruct.type == NICKNAME_INFOS)
	{

		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		printf("[INFO] : fd(%d) Getting infos for %s\n", client->fd, client->msgstruct.infos);
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
		
		printf("[INFO] : fd(%d) Message broadcasted : %s\n", client->fd, client->msg_content);
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
			printf("[INFO] : fd(%d) Tried to send message to non-existing user %s\n", client->fd, client->msgstruct.infos);
		} else {
			printf("[INFO] : fd(%d) Message sent to %s : %s\n", client->fd, client->msgstruct.infos, client->msg_content);
		}
	}

	if (client->msgstruct.type == MULTICAST_CREATE)
	{
		char content[MSG_LEN];

		//calling check_nickname for now but using neither return code nor content 
		int ok = check_channel_name(clients, client->msgstruct.infos, content, sizeof(content));

		
		struct message response;
		memset(&response, 0, sizeof(response));
		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = MULTICAST_CREATE;

		if (ok != 0)
		{
			snprintf(content, sizeof(content), "Invalid channel name : %s\n", client->msgstruct.infos);
			response.pld_len = strlen(content);
			printf("[INFO] : fd(%d) Tried to create invalid channel\n", client->fd);
		} else {

			char old_channel[CHANNEL_LEN];
			snprintf(old_channel, sizeof(old_channel), "%s", client->channel);

			leave_channel(*clients, client);
			if (old_channel[0] != '\0') // check if client was in a channel before
			{
				printf("[INFO] : fd(%d) leaving channel %s\n", client->fd, old_channel);

				char leave_msg[MSG_LEN];
				snprintf(leave_msg, sizeof(leave_msg), "%s has left the channel", client->nickname);

				notify_channel(*clients, old_channel, leave_msg, client);
			} 
			
			printf("[INFO] : fd(%d) Creating channel %s\n", client->fd, client->msgstruct.infos);		
			
			// join the created channel 
			snprintf(client->channel, sizeof(client->channel), "%.*s", CHANNEL_LEN - 1, client->msgstruct.infos);
			printf("[INFO] : fd(%d) Joining channel %s\n", client->fd, client->msgstruct.infos);		
				

			snprintf(content, sizeof(content), "You have created a channel : %s\n", client->msgstruct.infos);
			response.pld_len = strlen(content);

		}

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);

	}


	if (client->msgstruct.type == MULTICAST_LIST)
	{
		int found = 0;
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		snprintf(content, sizeof(content), "Existing channels :\n");
		for (struct Client *c = *clients; c != NULL; c = c->next)
		{

			//fix : removes duplicates in the list 
			int already_listed = 0;
			for (struct Client *p = *clients; p != c; p = p->next)
			{
				if (strcmp(p->channel, c->channel) == 0)   // p est avant c, même salon
				{
					already_listed = 1;
					break;
				}
			}

			//add only if no duplicates
			if (c->channel[0] != '\0' && !already_listed)
			{
				strncat(content, c->channel, sizeof(content) - strlen(content) - 1);
				strncat(content, "\n", sizeof(content) - strlen(content) - 1);
				found = 1;
			}
		}

		if (!found)
		{
			snprintf(content, sizeof(content) - strlen(content) - 1, "No channels available\n");
		}

		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = MULTICAST_LIST;
		response.pld_len = strlen(content);

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
		
	}

	if (client->msgstruct.type == MULTICAST_JOIN)
	{
		int exists = channel_exists(*clients, client->msgstruct.infos);
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));

		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = MULTICAST_JOIN;

		if (!exists)
		{
			snprintf(content, sizeof(content), "Channel %s does not exist\n", client->msgstruct.infos);
			response.pld_len = strlen(content);
			printf("[INFO] : fd(%d) Tried to join non-existing channel %s\n", client->fd, client->msgstruct.infos);
		} else {
			char old_channel[CHANNEL_LEN];
			snprintf(old_channel, sizeof(old_channel), "%s", client->channel);

			leave_channel(*clients, client);
			if (old_channel[0] != '\0') // check if client was in a channel before
			{
				printf("[INFO] : fd(%d) leaving channel %s\n", client->fd, old_channel);

				char leave_msg[MSG_LEN];
				snprintf(leave_msg, sizeof(leave_msg), "%s has left the channel", client->nickname);

				notify_channel(*clients, old_channel, leave_msg, client);
			} 
			
			printf("[INFO] : fd(%d) Joining channel %s\n", client->fd, client->msgstruct.infos);		
			
			snprintf(client->channel, sizeof(client->channel), "%.*s", CHANNEL_LEN - 1, client->msgstruct.infos);
				

			snprintf(content, sizeof(content), "You have joined the channel : %s\n", client->channel);
			response.pld_len = strlen(content);

			//notify other clients in the channel
			char join_msg[MSG_LEN];
			snprintf(join_msg, sizeof(join_msg), "%s has joined the channel", client->nickname);
			notify_channel(*clients, client->channel, join_msg, client);

		}

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);

	}

	if (client->msgstruct.type == MULTICAST_QUIT)
	{
		char channel_name[CHANNEL_LEN];
		snprintf(channel_name, sizeof(channel_name), "%s", client->channel);
		char content[MSG_LEN];
		struct message response;
		memset(&response, 0, sizeof(response));
		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = MULTICAST_QUIT;

		
		if (client->channel[0] == '\0') {
			snprintf(content, sizeof(content), "You are not in any channel\n");
			response.pld_len = strlen(content);
			printf("[INFO] : fd(%d) Tried to leave a channel but was not in any\n", client->fd);
		}else {
			int people_left = leave_channel(*clients, client);
			if (people_left == 0)
			{
				printf("[INFO] : fd(%d) Leaving channel %s, now deleted\n", client->fd, channel_name);
				snprintf(content, sizeof(content), "You have left the channel : %s, channel is now empty\n", channel_name);
			} else {
				char quit_msg[MSG_LEN];
				snprintf(quit_msg, sizeof(quit_msg), "%s has left the channel", client->nickname);
				notify_channel(*clients, channel_name, quit_msg, client);
				printf("[INFO] : fd(%d) Leaving channel %s\n", client->fd, channel_name);
				snprintf(content, sizeof(content), "You have left the channel : %s\n", channel_name);
			}
		}

		response.pld_len = strlen(content);

		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
	}

	if (client->msgstruct.type == FILE_REQUEST)
	{
		struct Client *target_client;
		char *filename;

		filename = client->msg_content;
		target_client = find_client_by_nickname(clients, client->msgstruct.infos);
		if (!target_client)
			not_found_response(client);
		else
		{
			char content[MSG_LEN];
			int len = snprintf(content, sizeof(content), "%s wants you to accept the transfer of the file named %s. Do you accept? [Y/N]\n", client->nickname, filename);
			if (len < 0 || (size_t)len >= sizeof(content))
			{
				fprintf(stderr, "Filename is too long\n");
				return 1;
			}
			// Transmettre la demande au recepteur
			struct message request = client->msgstruct;
			request.pld_len = len;
			snprintf(request.nick_sender, sizeof(request.nick_sender), "%s", client->nickname);
			die(send(target_client->fd, &request, sizeof(request), 0), "send message struct");
			die(send(target_client->fd, content, len, 0), "send message content");
			client->file_status = FILE_REQUESTED;
		}
	}

	if (client->msgstruct.type == FILE_ACCEPT || client->msgstruct.type == FILE_REJECT)
	{
		struct Client *target = find_client_by_nickname(clients, client->msgstruct.infos);
		if (!target)
			not_found_response(client);
		else
		{
			struct message fwd = client->msgstruct;
			snprintf(fwd.nick_sender, sizeof(fwd.nick_sender), "%s", client->nickname);
			send(target->fd, &fwd, sizeof(fwd), 0);
			send(target->fd, client->msg_content, fwd.pld_len, 0);
		}
	}

	return 1;
}
