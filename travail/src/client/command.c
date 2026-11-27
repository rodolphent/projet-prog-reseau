#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "command.h"
#include "common.h"
#include "file_transfer.h"
#include "msg_struct.h"

void send_message(int sockfd, const char *nickname, enum msg_type type,
				  const char *infos, const char *content)
{
	struct message msg;
	memset(&msg, 0, sizeof(msg));
	msg.pld_len = strlen(content);
	msg.type = type;
	snprintf(msg.nick_sender, sizeof(msg.nick_sender), "%s", nickname);
	snprintf(msg.infos, sizeof(msg.infos), "%s", infos);
	die(send(sockfd, &msg, sizeof(msg), 0), "send message struct");
	die(send(sockfd, content, msg.pld_len, 0), "send message content");
}

int open_listen_socket(int server_fd, char *out, size_t out_size)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	if (getsockname(server_fd, (struct sockaddr *)&addr, &len) < 0 || addr.sin_family != AF_INET)
		return -1;
	addr.sin_port = 0; // le noyau choisit le port

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	if (bind(fd, (struct sockaddr *)&addr, len) < 0 || listen(fd, 1) < 0 ||
		getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
	{
		close(fd);
		return -1;
	}

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
	snprintf(out, out_size, "%s:%d", ip, ntohs(addr.sin_port));
	return fd;
}

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
	}
	else if (strcmp(msg_type, "/msgall") == 0)
	{
		msg_type_res = BROADCAST_SEND;
	}
	else if (strcmp(msg_type, "/send") == 0)
	{
		msg_type_res = FILE_REQUEST;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/accept") == 0)
	{
		msg_type_res = FILE_ACCEPT;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/reject") == 0)
	{
		msg_type_res = FILE_REJECT;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/file") == 0)
	{
		msg_type_res = FILE_SEND;
		have_info = 1;
	}
	else if (strcmp(msg_type, "/ack") == 0)
	{
		msg_type_res = FILE_ACK;
		have_info = 1;
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

void handle_file_transfer_response(int sockfd, char *nickname, char *buffer, struct file_transfer *ft)
{
		buffer[strcspn(buffer, "\r\n")] = '\0'; // Enlever le retour à la ligne
		if (strcmp(buffer, "y") == 0 || strcmp(buffer, "Y") == 0)
		{
			char endpoint[64];
			ft->listen_fd = open_listen_socket(sockfd, endpoint, sizeof(endpoint));
			if (ft->listen_fd < 0)
			{
				perror("listen socket");
				send_message(sockfd, nickname, FILE_REJECT, ft->sender, "");
			}
			else
				send_message(sockfd, nickname, FILE_ACCEPT, ft->sender, endpoint);
		}
		else if (strcmp(buffer, "n") == 0 || strcmp(buffer, "N") == 0)
			send_message(sockfd, nickname, FILE_REJECT, ft->sender, "");
		else
		{
			printf("Please answer Y or N\n");
			return;
		}
		ft->pending = 0;
}

int handle_stdin_input(int sockfd, char *nickname, char *buffer, struct file_transfer *ft)
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

	//Gestion de la réponse à une demande de transfert de fichier
	if (ft->pending)
	{
		handle_file_transfer_response(sockfd, nickname, parsed_buffer, ft);
		return 1;
	}

	msg_type = get_msg_type_and_infos(parsed_buffer, &msg_infos, &msg_content);
	if (msg_type < 0)
	{
		fprintf(stderr, "Unknown command\n");
		return 1;
	}

	if (msg_type == FILE_REQUEST && !prepare_file_request(nickname, msg_infos, &msg_content, ft))
	{
		return 1;
	}

	struct message payload;
	payload.pld_len = strlen(msg_content);
	snprintf(payload.nick_sender, sizeof(payload.nick_sender), "%s", nickname);
	// payload.pld_len = n;
	payload.type = msg_type;
	size_t infos_len = strlen(msg_infos);
	if (infos_len >= sizeof(payload.infos))
	{
		fprintf(stderr, "Message information is too long\n");
		return 1;
	}
	memcpy(payload.infos, msg_infos, infos_len + 1);

	// Send the struct message first
	die(send(sockfd, &payload, sizeof(payload), 0), "send message struct");

	// Send the message content
	die(send(sockfd, msg_content, strlen(msg_content), 0), "send message content");

	printf("Message sent!\n");
	return 1;
}
