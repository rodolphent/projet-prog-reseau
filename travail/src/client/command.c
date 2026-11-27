#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "command.h"
#include "common.h"
#include "msg_struct.h"

int get_msg_type_and_infos(char *buffer,
						   char **msg_infos, char **msg_content)
{
	int i = 0;
	if (buffer == NULL || msg_infos == NULL || msg_content == NULL)
	{
		return -1;
	}

	while (isspace((unsigned char)*buffer))
	{
		buffer++;
	}

	char *line_end = buffer + strcspn(buffer, "\r\n");
	*line_end = '\0';
	*msg_infos = line_end;
	*msg_content = line_end;
	if (*buffer == '\0')
	{
		return -1;
	}

	if (*buffer != '/')
	{
		*msg_content = buffer;
		return ECHO_SEND;
	}

	char *msg_type = buffer;
	while (*buffer != '\0' && !isspace((unsigned char)*buffer))
	{
		buffer++;
	}
	if (*buffer != '\0')
	{
		*buffer++ = '\0';
	}
	while (isspace((unsigned char)*buffer))
	{
		buffer++;
	}
	*msg_content = buffer;

	int msg_type_res;
	int have_info = 0;
	if (strcmp(msg_type, "/nick") == 0)
	{
		msg_type_res = NICKNAME_NEW;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/list") == 0)
	{
		msg_type_res = NICKNAME_LIST;
	}
	else if (strcmp(msg_type, "/info") == 0)
	{
		msg_type_res = NICKNAME_INFOS;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/msg") == 0)
	{
		msg_type_res = UNICAST_SEND;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/broadcast") == 0)
	{
		msg_type_res = BROADCAST_SEND;
	}
	else if (strcmp(msg_type, "/who") == 0)
	{
		msg_type_res = NICKNAME_LIST;
	}
	else if (strcmp(msg_type, "/whois") == 0)
	{
		msg_type_res = NICKNAME_INFOS;
		have_info = 1;
	}else if (strcmp(msg_type, "/msgall")==0)
	{
		msg_type_res = BROADCAST_SEND;
	}
	else if (strcmp(msg_type, "/create") == 0)
	{
		msg_type_res = MULTICAST_CREATE;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/channel_list") == 0)
	{
		msg_type_res = MULTICAST_LIST;
	}
	else
	{
		return -1;
	}

	if (have_info)
	{
		*msg_infos = buffer;
		while (*buffer != '\0' && !isspace((unsigned char)*buffer) && i < INFOS_LEN - 1)
		{
			i++;
			buffer++;
		}
		if (*buffer != '\0')
		{
			*buffer++ = '\0';
		}
		if (i >= INFOS_LEN - 1)
		{
			fprintf(stderr, "Message information is too long\n");
			return -1;
		}
		while (isspace((unsigned char)*buffer))
		{
			buffer++;
		}
		*msg_content = buffer;
	}

	return msg_type_res;
}

int handle_stdin_input(int sockfd, char* nickname, char *buffer)
{
	int n = read(STDIN_FILENO, buffer, MSG_LEN - 1);
	char *msg_infos;
	char *msg_content;
	int msg_type;
	if (n < 0)
	{
		perror("read stdin");
		return 0;
	}
	if (n == 0)
	{
		printf("stdin closed\n");
		return 0;
	}

	char parsed_buffer[MSG_LEN];
	memcpy(parsed_buffer, buffer, (size_t)n);
	parsed_buffer[n] = '\0';

	msg_type = get_msg_type_and_infos(parsed_buffer, &msg_infos, &msg_content);
	if (msg_type < 0)
	{
		fprintf(stderr, "Unknown command\n");
		return 1;
	}

	struct message payload;
	payload.pld_len = strlen(msg_content);
	snprintf(payload.nick_sender, sizeof(payload.nick_sender), "%s", nickname);
	//payload.pld_len = n;
	payload.type = msg_type;
	size_t infos_len = strlen(msg_infos);
	if (infos_len >= sizeof(payload.infos))
	{
		fprintf(stderr, "Message information is too long\n");
		return 1;
	}
	memcpy(payload.infos, msg_infos, infos_len + 1);


	//Send the struct message first 
	die(send(sockfd, &payload, sizeof(payload), 0), "send message struct");

	//Send the message content
	die(send(sockfd, msg_content, strlen(msg_content), 0), "send message content");

	//printf("Message sent!\n");
	return 1;
}
