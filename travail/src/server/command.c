#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "common.h"
#include "server_command.h"

int handle_command( struct Client* client)
{

	//TODO : handle quit 
	if (client->msgstruct.type == 9999)
	{
		printf("quitting\n");
		return 0;
	}

	if (client->msgstruct.type == NICKNAME_NEW)
	{
		
		//TODO : check if nick is valid before accepting it

		printf("nickname changed for fd(%d) from %s to %s\n", client->fd, client->nickname,client->msgstruct.infos);
		snprintf(client->nickname, NICK_LEN, "%s", client->msgstruct.infos);
		//TODO : also check for non ascii characters
		struct message response;
		memset(&response,0,sizeof(response));
		snprintf(response.nick_sender, sizeof(response.nick_sender), "%s", "Server");
		response.type = NICKNAME_NEW;
		char content[MSG_LEN];
		snprintf(content, sizeof(content), "Welcome to the server %s !\n", client->nickname);
		response.pld_len = strlen(content);
		snprintf(response.infos, sizeof(response.infos), "%s", client->nickname);
		
		
		send(client->fd, &response, sizeof(response), 0);
		send(client->fd, content, strlen(content), 0);
		
	}

	if (client->msgstruct.type == ECHO_SEND) {
		//TODO : echo things

		send(client->fd, &client->msgstruct, sizeof(client->msgstruct), 0);
		send(client->fd, client->msg_content, strlen(client->msg_content), 0) ;
		
		printf("Message sent!\n");
	}
	return 1;
}
